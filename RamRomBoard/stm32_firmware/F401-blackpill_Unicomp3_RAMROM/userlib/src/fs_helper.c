/*
 * fs_helper.c
 *
 *  Created on: Sep 26, 2023
 *      Author: rob
 */
#include "fs_helper.h"
#include "portab.h"
/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/
/**
 * @brief FS object.
 */
extern bool fs_ready;
extern FATFS SDC_FS;
extern const ShellConfig shell_cfg1;
//extern event_source_t autorun_terminated;
extern thread_t *artp;
extern bool autorun;
extern uint8_t secdata[256];
extern BaseSequentialStream *const dbg;
DIR Dir;  /* Directory object */
/* Generic large console buffer.*/
static char fbuff[512];
static char arbuf[256];
extern const MMCConfig portab_mmccfg;
extern MMCDriver MMCD1;

FILINFO Finfo;
FIL fsrc, fdest, far;

static FRESULT scan_files(BaseSequentialStream *chp, char *path) {
  static FILINFO fno;
  FRESULT res;
  DIR dir;
  size_t i;
  char *fn;

  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    while (((res = f_readdir(&dir, &fno)) == FR_OK) && fno.fname[0]) {
      if (FF_FS_RPATH && fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        *(path + i) = '/';
        strcpy(path + i + 1, fn);
        res = scan_files(chp, path);
        *(path + i) = '\0';
        if (res != FR_OK)
          break;
      }
      else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

char* fresult_str(FRESULT stat) {
  char str[255];
  memset(str,0,sizeof(str));
  switch (stat) {
  case FR_OK:
    return "Succeeded";
  case FR_DISK_ERR:
    return "A hard error occurred in the low level disk I/O layer";
  case FR_INT_ERR:
    return "Assertion failed";
  case FR_NOT_READY:
    return "The physical drive cannot work";
  case FR_NO_FILE:
    return "Could not find the file";
  case FR_NO_PATH:
    return "Could not find the path";
  case FR_INVALID_NAME:
    return "The path name format is invalid";
  case FR_DENIED:
    return "Access denied due to prohibited access or directory full";
  case FR_EXIST:
    return "Access denied due to prohibited access";
  case FR_INVALID_OBJECT:
    return "The file/directory object is invalid";
  case FR_WRITE_PROTECTED:
    return "The physical drive is write protected";
  case FR_INVALID_DRIVE:
    return "The logical drive number is invalid";
  case FR_NOT_ENABLED:
    return "The volume has no work area";
  case FR_NO_FILESYSTEM:
    return "There is no valid FAT volume";
  case FR_MKFS_ABORTED:
    return "The f_mkfs() aborted due to any parameter error";
  case FR_TIMEOUT:
    return "Could not get a grant to access the volume within defined period";
  case FR_LOCKED:
    return "The operation is rejected according to the file sharing policy";
  case FR_NOT_ENOUGH_CORE:
    return "LFN working buffer could not be allocated";
  case FR_TOO_MANY_OPEN_FILES:
    return "Number of open files > _FS_SHARE";
  case FR_INVALID_PARAMETER:
    return "Given parameter is invalid";
  default:
    return "Unknown";
  }
  return "";
}

/**
 * xtou16
 * Take a hex string and convert it to a 64bit number (max 16 hex digits).
 * The string must only contain digits and valid hex characters.
 */
uint16_t xtou16(const char *str){
  uint16_t res = 0;
  char c;
  //chprintf(dbg, "String: %s\r\n", str);
  while ((c = *str++)) {
    char v = (((c & 0xF) + (c >> 6)) | ((c >> 3) & 0x8));
    res = (res << 4) | (uint16_t) v;
  }

  return res;
}

uint16_t get_number(const char * str){
  uint8_t cnt = 0;
  //chprintf(dbg, "Copy %s\r\n", str);
  while (str[cnt] == ' '){
    cnt++;
    //chprintf(dbg, "Whitespace found\r\n");
  }
  if (str[cnt] == '$'){
    //chprintf(dbg, "$ detected\r\n");
    return xtou16(&(str[cnt+1]));
  }
  else if ((str[cnt+1] == 'x') || (str[cnt+1] == 'X')){
    //chprintf(dbg, "0x detected\r\n");
    return xtou16(&(str[cnt+2]));
  }
  else{
    return (uint16_t)atoi(str);
    //chprintf(dbg, "Number detected\r\n");
  }
}

void verbose_error(BaseSequentialStream *chp, FRESULT err) {
  chprintf(chp, "\t%s.\r\n",fresult_str(err));
}

void cmd_tree(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t fre_clust;
  FATFS *fsp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
    return;
  }

  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }

  err = f_getfree("/", &fre_clust, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed\r\n");
    return;
  }
  chprintf(chp,
           "FS: %lu free clusters with %lu sectors (%lu bytes) per cluster\r\n",
           fre_clust, (uint32_t)fsp->csize, (uint32_t)fsp->csize * 512);
  fbuff[0] = 0;
  scan_files(chp, (char *)fbuff);
}

void cmd_create(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  FIL fil;
  UINT btw, chunk;
  UINT bw;
  const uint8_t data[] = {0,0,0,0,0,0,0,0};

  if (argc != 2) {
    chprintf(chp, "Usage: create <name> <size in byte>\r\n");
    chprintf(chp, "       creates binary file with name and size.\r\n");
    return;
  }

  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }
  btw = atoi(argv[1]);

  err = f_open(&fil, (const TCHAR *)argv[0], FA_CREATE_ALWAYS | FA_WRITE);
  if (err) { verbose_error(dbg, err); return; }
  do {
    if (btw <= sizeof(data)){ 
        chunk = btw;
        btw = 0;
    }
    else { 
      chunk = sizeof(data);
      btw -= chunk;
    }
    err = f_write(&fil, (const void *)data, chunk, &bw);
    if (err || bw < chunk) break;   /* error or eof */
  } while (btw);

  err = f_close(&fil);
  if (err) { verbose_error(dbg, err); return; }
}


/*
 * Print a text file to screen
 */
void cmd_cat (BaseSequentialStream *chp, int argc, char *argv[]) {
  /*
   * Print usage
   */
  if (argc != 1) {
    chprintf(chp, "Usage: cat filename\r\n");
    chprintf(chp, "       Echos filename (no spaces)\r\n");
    return;
  }
  FRESULT err;
  char Buffer[80];
  uint16_t i;
  UINT ByteToRead=sizeof(Buffer);
  UINT ByteRead;
  /*
   * Attempt to open the file, error out if it fails.
   */
  chprintf(dbg, "Filename: %s\r\n", argv[0]);
  err=f_open(&fsrc, argv[0], FA_READ);
  if (err != FR_OK) {
      chprintf(chp, "FS: f_open(%s) failed.\r\n", argv[0]);
      verbose_error(chp, err);
      return;
  }

  /*
    * Do while the number of bytes read is equal to the number of bytes to read
    * (the buffer is filled)
    */
   do {
       memset(Buffer,0,sizeof(Buffer));
       err=f_read(&fsrc,Buffer,ByteToRead,&ByteRead);
       if (err) { verbose_error(chp, err); f_close(&fsrc); return; }
       for (i = 0; i < ByteRead; i++){
           if ((Buffer[i] != 0x0a) && (Buffer[i] != 0x0c)){
               chprintf(chp, "%c", Buffer[i]);
           }
           else chprintf(chp, "\r\n");
       }
   } while (ByteRead >= ByteToRead);
   //chprintf(chp, "%s", Buffer);
   chprintf(chp,"\r\n");
   f_close(&fsrc);
  return;
}

static char *parse_arguments(char *str, char **saveptr) {
  char *p;

  if (str != NULL)
    *saveptr = str;

  p = *saveptr;
  if (!p) {
    return NULL;
  }

  /* Skipping white space.*/
  p += strspn(p, " \t");

  if (*p == '"') {
    /* If an argument starts with a double quote then its delimiter is another
       quote.*/
    p++;
    *saveptr = strpbrk(p, "\"");
  }
  else {
    /* The delimiter is white space.*/
    *saveptr = strpbrk(p, " \t");
  }

  /* Replacing the delimiter with a zero.*/
  if (*saveptr != NULL) {
    *(*saveptr)++ = '\0';
  }

  return *p != '\0' ? p : NULL;
}

static int16_t my_getline(char *lbuf, char *lc, uint16_t * pos){
    int16_t r=40, keep = 1;
    if ((lbuf[*pos] == '#') || 
        (lbuf[*pos] == ';') ||
        (lbuf[*pos] == '*') || 
        (lbuf[*pos] == '*')) keep = 0; //Comment Line, go to end

    if ((lbuf[*pos] == '\r') || 
        (lbuf[*pos] == '\n')){  //Another Lineend
            (*pos)++;
            return 1;
    }

    //printf("char: %c off: %i\n", lbuf[*pos], *pos);
    do {
        if ((lbuf[*pos] == '\n') || (lbuf[*pos] == '\r') || (lbuf[*pos] == '\0')){
            break;  
        } 
        if (keep) *lc++ = lbuf[*pos];
        (*pos)++;
    } while (--r);
    if (keep) return 0;
    else return 1;
}

static bool cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
                      char *name, int argc, char *argv[]) {

  while (scp->sc_name != NULL) {
    if (strcmp(scp->sc_name, name) == 0) {
      scp->sc_function(chp, argc, argv);
      return false;
    }
    scp++;
  }
  return true;
}

void dump_argv(char * cmd, int argc, char * argv[]){
    int i;
    chprintf(dbg, "%s", cmd);
    //chThdSleepMilliseconds(10);
    //chprintf(dbg, "argc: %i\r\n", argc);
    //chThdSleepMilliseconds(10);
    //if (argc == 0) chprintf(dbg, "\r\n");
    for (i=0; i<argc; i++){
        chprintf(dbg, " %s", argv[i]);
        //chThdSleepMilliseconds(10);
    }
    chprintf(dbg, "\r\n");
}


//buf = &arbuf[0];

uint16_t get_file(BaseSequentialStream *chp, char * fn){
    FRESULT err;
    UINT fsize;
    memset(arbuf,0,sizeof(arbuf)); //Clear the filebuffer.

    err = f_open(&far, fn, FA_OPEN_EXISTING | FA_READ);
    if (err ) { verbose_error(dbg, err); return 0; }  /* error or eof */
    
    err = f_read(&far, arbuf, sizeof(arbuf), &fsize);
    if (err ) { verbose_error(dbg, err); return 0; }  /* error or eof */
    
    chprintf(chp, "%u Bytes copied\r\n",fsize);
    f_close(&far);
    // Data is now in arbuf
    return (uint16_t)fsize;
}


void parse_buffer(const ShellConfig *scfg, uint16_t fsize){
    //ShellConfig *scfg = p;
char line[40]; // 40 characters per line max!
 //   char * line = &lbuf[0];
    char * arbufp = &arbuf[0];
    BaseSequentialStream *chp = scfg->sc_channel;
    const ShellCommand *scp = scfg->sc_commands;
    
    int n;
    uint16_t position = 0;
    
    char *lp, *cmd, *tokp;
    char *args[SHELL_MAX_ARGUMENTS + 1];

    do{
        memset(line,0,sizeof(line));
        if (my_getline(arbufp, line, &position) == 0){

            lp = parse_arguments(line, &tokp);
            cmd = lp;
            n = 0;
            while ((lp = parse_arguments(NULL, &tokp)) != NULL) {
              if (n >= SHELL_MAX_ARGUMENTS) {
                
                chprintf(chp, "too many arguments\r\n");
                cmd = NULL;
                break;
              }
              args[n++] = lp;
            }
            args[n] = NULL;
            dump_argv(cmd, n, args);
            cmdexec(scp, chp, cmd, n, args);
        }
    } while (position < fsize);
}

/*
 * Executes a script
 */

void cmd_exec(BaseSequentialStream *chp, int argc, char *argv[]) {
    uint16_t fsize;
    //char * arbuf=&arbuf[0];
    const ShellConfig *scfg = &shell_cfg1;
    if (argc != 1) {
        chprintf(chp, "Usage: exec scriptname\r\n");
        chprintf(chp, "       Runs a script\r\n");
        return;
    }
    chprintf(dbg, "Filename: %s\r\n", argv[0]);
    
    fsize = get_file(chp, argv[0]);
    if (fsize > 0) {
        parse_buffer(scfg, fsize);
    }
    return;
}

uint8_t exec(ShellConfig *scfg){
    //char * arbuf=&arbuf[0];
    //ShellConfig *scfg = p;
    BaseSequentialStream *chp = scfg->sc_channel;    
    uint16_t fsize;
    //char * arp[2] = {"autorun.sh", NULL};

    fsize = get_file(chp, "autorun.sh");
    if (fsize > 0){
        parse_buffer(scfg, fsize);
        return 1;
    } 
    return 2;
}

void cmd_cd(BaseSequentialStream *chp, int argc, char *argv[]){
  FRESULT err;
  if (argc != 1) {
    chprintf(chp, "Usage: cd dirName\r\n");
    chprintf(chp, "       Changes directory \r\n");
    return;
  }
  err=f_chdir(argv[0]);
  if (err != FR_OK) {
    /*
     * Display failure message and reason.
     */
    chprintf(chp, "FS: f_mkdir(%s) failed\r\n",argv[0]);
    verbose_error(chp, err);
    return;
  } else {
    err = f_getcwd(fbuff, sizeof fbuff);
    if (err) { verbose_error(chp, err); return; }
    chprintf(chp, "%s\r\n", fbuff);
    //chprintf(chp, "FS: f_mkdir(%s) succeeded\r\n",argv[0]);
  }
  return;
}

void cmd_pwd(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argc;
  (void)argv;
  FRESULT err;
  err = f_getcwd(fbuff, sizeof fbuff);
  if (err) { verbose_error(chp, err); return; }
  chprintf(chp, "%s\r\n", fbuff);
  return;
}

void cmd_mkdir(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  if (argc != 1) {
    chprintf(chp, "Usage: mkdir dirName\r\n");
    chprintf(chp, "       Creates directory with dirName (no spaces)\r\n");
    return;
  }
  /*
   * Attempt to make the directory with the name given in argv[0]
   */
  err=f_mkdir(argv[0]);
  if (err) { verbose_error(chp, err); return; }
  chprintf(chp, "FS: f_mkdir(%s) succeeded\r\n",argv[0]);
  return;
}

void cmd_rmdir(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  if (argc != 1) {
    chprintf(chp, "Usage: rmdir dirname\r\n");
    chprintf(chp, "       Deletes directory with dirname (no spaces)\r\n");
    return;
  }
  err=f_unlink(argv[0]);
  if (err) { verbose_error(chp, err); return; }
  chprintf(chp, "Directory %s deleted.\r\n", argv[0]);
  return;
}

void cmd_rm(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  if (argc != 1) {
    chprintf(chp, "Usage: rm filename\r\n");
    chprintf(chp, "       Deletes file with filename (no spaces)\r\n");
    return;
  }
  err=f_unlink(argv[0]);
  if (err) { verbose_error(chp, err); return; }
  chprintf(chp, "File %s deleted.\r\n", argv[0]);
  return;
}

void cmd_mv(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  if (argc != 2) {
    chprintf(chp, "Usage: mv oldfilename newfilename\r\n");
    chprintf(chp, "       Moves oldfilename to newfilename.\r\n");
    return;
  }
  err=f_rename(argv[0], argv[1]);
  if (err) { verbose_error(chp, err); return; }
  chprintf(chp, "File %s renamed to %s.\r\n", argv[0], argv[1]);
  return;
}

void cmd_free(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t dw;
  (void)argc;
  (void)argv;
  err = f_getfree("/", &dw, NULL);
    if (err == FR_OK) {
      chprintf(chp,"FS: %lu free clusters\r\n    %lu sectors per cluster\r\n",
            dw, (uint32_t)SDC_FS.csize);
        chprintf(chp,"%lu bytes free\r\n",
            dw * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
        chprintf(chp,"%lu KB free\r\n",
            (dw * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE)/(1024));
        chprintf(chp,"%lu MB free\r\n",
            (dw * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE)/(1024*1024));
    } else {
      verbose_error(chp, err);
    }
  }

void cmd_mount(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  //DIR dir;
  (void)argc;
  (void)argv;

  if (mmcConnect(&MMCD1))
    return;

  /*
   * Attempt to mount the drive.
   */
  //err = f_mount(0, &SDC_FS);
  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_mount() failed. Is the SD card inserted?\r\n");
    verbose_error(chp, err);
    return;
  }
  chprintf(chp, "FS: f_mount() succeeded\r\n");
  //mmcConnect(&MMCD1);
  fs_ready = TRUE;
  autorun = TRUE;
  chSysLock();
  //chThdResumeI(&artp, MSG_OK);
  chSysUnlock();
}


void cmd_umount(BaseSequentialStream *chp, int argc, char *argv[]){
  FRESULT err;
  (void)argc;
  (void)argv;
  fs_ready = FALSE;
  autorun = FALSE;
  mmcDisconnect(&MMCD1);
  err = f_mount(NULL, "/", 0);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_mount() unmount failed\r\n");
    verbose_error(chp, err);
    return;
  }
  return;
}

void cmd_setlabel(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  if (argc != 1) {
    chprintf(chp, "Usage: setlabel label\r\n");
    chprintf(chp, "       Sets FAT label (no spaces)\r\n");
    return;
  }
  /*
   * Attempt to set the label with the name given in argv[0].
   */
  err=f_setlabel(argv[0]);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_setlabel(%s) failed.\r\n",argv[0]);
    verbose_error(chp, err);
    return;
  } else {
    chprintf(chp, "FS: f_setlabel(%s) succeeded.\r\n",argv[0]);
  }
  return;
}

void cmd_getlabel(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  char lbl[12];
  DWORD sn;
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: getlabel\r\n");
    chprintf(chp, "       Gets and prints FAT label\r\n");
    return;
  }
  memset(lbl,0,sizeof(lbl));
  /*
   * Get volume label & serial of the default drive
   */
  err = f_getlabel("", lbl, &sn);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getlabel failed.\r\n");
    verbose_error(chp, err);
    return;
  }
  /*
   * Print the label and serial number
   */
  chprintf(chp, "LABEL: %s\r\n",lbl);
  chprintf(chp, "  S/N: 0x%X\r\n",sn);
  return;
}


void cmd_copy(BaseSequentialStream *chp, int argc, char *argv[]){
  if (argc != 2) {
    chprintf(chp, "Usage: cp sourcefile destinationfile\r\n");
    chprintf(chp, "      Copies file to file.\r\n");
    return;
  }
  FRESULT err;

  uint32_t p1;
  UINT s1, s2;

  err = f_open(&fsrc, argv[0], FA_OPEN_EXISTING | FA_READ);
  if (err) { verbose_error(dbg, err); return; }
  err = f_open(&fdest, argv[1], FA_CREATE_ALWAYS | FA_WRITE);
  if (err) { verbose_error(dbg, err); return; }
  chprintf(chp, "Copy from %s to %s....\r\n", argv[0], argv[1]);
  for (;;) {
    err = f_read(&fsrc, fbuff, sizeof(fbuff), &s1);
    if (err || s1 == 0) break;   /* error or eof */
    err = f_write(&fdest, fbuff, s1, &s2);
    p1 += s2;
    if (err || s2 < s1) break;   /* error or disk full */
  }
  chprintf(chp, "%u Bytes copied\r\n",p1);
  f_close(&fsrc);
  f_close(&fdest);
}

void cmd_dir(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argc;
  (void)argv;
  FRESULT err;
  //FATFS *fs;
  uint32_t acc_size,dw;
  uint16_t acc_dirs, acc_files;
  err = f_getcwd(fbuff, sizeof fbuff);
  if (err) { verbose_error(chp, err); return; }
  err = f_opendir(&Dir, fbuff);
  if (err) { verbose_error(chp, err); return; }
  acc_size = acc_dirs = acc_files = 0;
  for(;;) {
    err = f_readdir(&Dir, &Finfo);
    if ((err != FR_OK) || !Finfo.fname[0]) break;
    if (Finfo.fattrib & AM_DIR) {
      acc_dirs++;
    }
    else {
      acc_files++; acc_size += Finfo.fsize;
    }
    chprintf(chp, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
            (Finfo.fattrib & AM_DIR) ? 'D' : '-',
                (Finfo.fattrib & AM_RDO) ? 'R' : '-',
                    (Finfo.fattrib & AM_HID) ? 'H' : '-',
                        (Finfo.fattrib & AM_SYS) ? 'S' : '-',
                            (Finfo.fattrib & AM_ARC) ? 'A' : '-',
                                (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
                                (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
                                Finfo.fsize, Finfo.fname);
  }
  chprintf(chp, "%4u File(s),%10u bytes total\r\n%4u Dir(s)", acc_files, acc_size, acc_dirs);
  err = f_getfree("/", &dw, NULL);
  if (err == FR_OK) {
    chprintf(chp, ", %10u bytes free\r\n", (uint32_t)dw * SDC_FS.csize * 512);
  } else {
    verbose_error(chp, err);
  }
}

void cmd_kar(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argc;
  (void)argv;
  (void)chp;
//chThdResumeI(&artp, MSG_OK);
autorun = TRUE;
//    chSysLock();
//    chEvtBroadcastI(&autorun_terminated);
//    chThdExitS(MSG_OK);

}
