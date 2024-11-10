#!/usr/bin/env python3
import sys, os, enum
from math import log, ceil
import argparse, time
from serial import Serial
from serial import SerialException
from configobj import ConfigObj
#import pickle

# UC3_upload.py
# This is the upload script for the Unicom Project.
# It is for Unicomp3!
#
# Version History:
# v1.0: Initial Version

ver = '3.0'
port = '/dev/ttyACM0'

#RAMSIZE = 0x80000 # 512kB RAM
RAMSIZE = 0x100000 # 1MB RAM
#RAMSIZE = 0x10000 # 64kB RAM
BLOCKSIZE = 8192 # Chunksize for read
DEBUG = 4 # 1: Clock, 2: transmission, 3: config, 4: RAM, 5: reset, 6: tic, 7: xsvf

# These Values come from upload.h
RAMR = 123
CLOCKR = 124
VERSION = 125
#STATUS = 126
#PROGRESS = 127
#NO_FUNC = 128
XSVF = 129
TRESETW = 192
CLOCKW = 193
CONFIGW = 194
TICW = 195
RAMW = 196

class bcolors:
	FAIL = '\033[91m'    #red
	OKGREEN = '\033[92m' #green
	WARNING = '\033[93m' #yellow
	OKBLUE = '\033[94m'  #dblue
	HEADER = '\033[95m'  #purple
	OKCYAN = '\033[96m'  #cyan
	ENDC = '\033[0m'
	BOLD = '\033[1m'
	UNDERLINE = '\033[4m'
#print(f'{bcolors.FAIL}{bcolors.ENDC}')
#print(f'{bcolors.OKGREEN}{bcolors.ENDC}')
#print(f'{bcolors.WARNING}{bcolors.ENDC}')
#print(f'{bcolors.OKBLUE}{bcolors.ENDC}')
#print(f'{bcolors.HEADER}{bcolors.ENDC}')
#print(f'{bcolors.OKCYAN}{bcolors.ENDC}')

class x_state(enum.Enum):
    XCOMPLETE    = 0  # 0
    XTDOMASK     = 1  # 1
    XSIR         = 2  # 2
    XSDR         = 3  # 3
    XRUNTEST     = 4  # 4
    XREPEAT      = 7  # 7
    XSDRSIZE     = 8  # 8
    XSDRTDO      = 9  # 9
    XSETSDRMASKS = 10 # A
    XSDRINC      = 11 # B
    XSDRB        = 12 # C
    XSDRC        = 13 # D
    XSDRE        = 14 # E
    XSDRTDOB     = 15 # F
    XSDRTDOC     = 16 # 10
    XSDRTDOE     = 17 # 11
    XSTATE       = 18 # 12
    XSIR2       = 254
    XIDLE       = 255

##########################################
def dict_to_bytearray(dct):
	retval = bytearray()
	#return bytearray(pickle.dumps(dct))
	for a in dct:
		#print(f'A: {a}, {dct[a]}')
		retval += a.to_bytes(3, 'big')  # Address
		retval += dct[a].to_bytes(2, 'big') # Chipselect
	return retval
#------------------------------------------
def dump_data(data, width):
	idx = len(data)
	for w in range(width):
		print (f'{bcolors.OKCYAN}  {w:02} ', end = '')
	print(f'{bcolors.ENDC}')
	for i in range(0, len(data), width):
		#print(f'idx: {idx}')
		if idx < width:
			stop = idx
		else:
			stop = width
		for j in range(0,stop):
			print(f'{bcolors.WARNING}0x{data[i+j]:02X} ', end = '')
		idx -= width
		if (idx >= width):
			print(f'{bcolors.ENDC} - {(i+width):>2} ({(i+width):02X})\r')
		else:
			print(f'{bcolors.ENDC}')
		#print(f'{bcolors.ENDC}\r')
#------------------------------------------
def read_file(fn):
	try:
		with open(fn, "rb") as f:
			#print('File open')
			img = f.read()
			f.close()
			return img
	except Exception as e:
		print(f'File {fn} not found!')
		exit()
#----------------------------------
def write_file(data, fn):
	if fn == '':
		dump_data(data, 16)
		exit()
	else:
		with open(fn, "wb") as f:
			#print(f'File open')
			f.write(bytes(data))
			f.close()
#----------------------------------
def find_registers(freq_fast, freq_slow=1000000):
	#rangereg = 18 # DS1085L-5
	rangereg = 15 # DS1085Z-25
	offset = 6
	p0 = 1
	minfreq = 33E6 # for DS1085L
	minfreq = 66E6 # for DS1085Z
	#stepsize = 5000 # Datasheet 5kHz Stepsize for DS1085L-5
	#offsetsize = 2560000 # 2560000 is offset size from datasheet for DS1085L-5
	stepsize = 25000 # Datasheet 25kHz Stepsize for DS1085L-25 and DS1085Z-25
	#stepsize = 12500 # Datasheet 12.5kHz Stepsize for DS1085L-12
	#offsetsize = 3200000 # 3200000 is offset size from datasheet for DS1085L-25 and DS1085L-12
	offsetsize = 6400000 # 6400000 is offset size from datasheet for DS1085Z-25 and DS1085Z-50
	dacmax = 1023   # Datasheet DAC can be 0-1023
	mclk_window_max = (int)((offsetsize * (offset + rangereg) + (dacmax*stepsize)) / p0)
	mclk_window_min = (int)((offsetsize * (offset + rangereg)) / p0)
	while freq_fast <= mclk_window_max:
		#print (f'1st freq window: {mclk_window_max} - {mclk_window_min}, offset: {offset}, p0: {p0}')
		if freq_fast >= mclk_window_min:
			break
		offset -= 1
		if offset == -7:
			offset = 6
			p0 *= 2
		if p0 > 8:
			print("too small, out of Range!")
			exit()
		mclk_window_max = (int)((offsetsize * (offset + rangereg) + (dacmax*stepsize)) / p0)
		mclk = (int)(offsetsize * (offset + rangereg))
		mclk_window_min = (int)((offsetsize * (offset + rangereg)) / p0)
		if (mclk_window_min < minfreq) and (p0 < 8):
			mclk_window_min = int(minfreq / p0)
		if (DEBUG == 1):
			print (f'freq window: {mclk_window_max} - {mclk_window_min}, offset: {offset}, p0: {p0}')
	dac_off = round((freq_fast - mclk_window_min) * p0 / stepsize)
	real_freq = (int)((offsetsize * (offset + rangereg) + (dac_off*stepsize)) / p0)
	error = 1E6 * ((real_freq - freq_fast) / freq_fast)
	main_clock = real_freq * p0
	print(f"found offset: {offset}, prescler0: {p0}, DAC: {dac_off}, Main Clockfreq.: {main_clock}")	
	if main_clock < minfreq:
		print(f'{bcolors.FAIL}')
		print(f'****************************************************************')
		print(f'****************************************************************')
		print(f'***********************  ATTENTION!  ***************************')
		print(f'***********  a main clock frequency below {int(minfreq/1E6)}MHz can  ***********')
		print(f'****************  lead to unexpected operation!  ***************')
		print(f'****************************************************************')
		print(f'****************************************************************')
		print(f'{bcolors.ENDC}')
	print(f"frequency fast in: {freq_fast} out: {real_freq} CPU: {real_freq/8} Hz Error: {error:.2f} ppm")
	div1 = (round(main_clock / freq_slow)) - 2
	if div1 > 1024:
		print(f'{bcolors.FAIL}ATTETION DIVIDER > 1024!{bcolors.ENDC}')
	real_freq2 = main_clock / (div1 + 2)
	error2 = 1E6 * ((real_freq2 - freq_slow) / freq_slow)
	error3 = 100 * ((real_freq2 - freq_slow) / freq_slow)
	print(f'frequency slow in: {freq_slow} out: {int(real_freq2)}, divider: {div1} Error: {error3:#.3f} %, {error2:#.3f} ppm')
	return offset,p0,dac_off,div1
#----------------------------------
def make_checksum(data):
	return sum(data) & 0xff
#----------------------------------
def write(channel, data):
	if (channel == None):
		return
	if isinstance(data, int):
		data = bytes((data,))

	try:
		result = channel.write(data)
		#print(f'Write Result: {result}')
		if result != len(data):
			raise Exception('write timeout')
		channel.flush()
	except:
		print(f'{bcolors.FAIL}Write error!{bcolors.ENDC}')
		#print(f'Write: {data}')
		#[print(f'Write: {e:02X}, {chr(e)}') for e in data]
		pass
#----------------------------------
def read(channel, size = 1):
	if (channel == None):
		return bytearray.fromhex('00')
	# Read a sequence of bytes
	if size == 0:
		return

	try:
		result = channel.read(size)
		#print(f'Read Result length: {len(result)}')
		if len(result) != size:
			print(f'Read error, Size: 0x{result:02X}')
			raise Exception('Read error')
	except:
		print(f'{bcolors.FAIL}Read error!         {bcolors.ENDC}')
		#result = (read_file('rom.bin'))[:size] #only for debug on a pc
	return result
#----------------------------------
def expect_ok(ser):
	if ser == None:
		return
	response = read(ser)
	if response == b'X':
		print(f'Checksum or Programming Error')
	elif response == b'O':
		if (DEBUG == 2):
			print(f'Response OK.')
	else:
		#raise Exception('Response error')
		print(f'{bcolors.FAIL}Response error!{bcolors.ENDC}')
#----------------------------------
def expect_done(ser):
	if ser == None:
		return
	response = read(ser)
	if response == b'X':
		print(f'Checksum or Programming Error')
	elif response == b'Y':
		#print(f'{bcolors.OKGREEN}Upload done.{bcolors.ENDC}')
		pass
	else:
		raise Exception('Response error')
#----------------------------------
def write_with_checksum(ser, data):
	cs_file = make_checksum(data)
	data += bytes([cs_file])
	write(ser, data)
#----------------------------------
def read_with_checksum(ser, size):
	data = read(ser, size)
	checksum = ord(read(ser, 1))
	cs_file = make_checksum(data)
	if (DEBUG == 2):
		print(f'Checksum: 0x{checksum:02x}')
	if checksum != cs_file:
		#pass # TODO raise Exception('Read checksum does not match')
		print(f'Checksum does not match! 0x{checksum:02x} vs 0x{cs_file:02x}')
	else:
		if (DEBUG > 0):
			print(f'{bcolors.OKGREEN}Readchecksum OK.{bcolors.ENDC}')
	return data
#----------------------------------
def dump_registers(result):
	if result[0] != None:
		print(f'-- Range Register: {result[0]}')
	if result[1] != None:
		print(f'-- Offset Register: {result[1]}')
	if result[2] != None:
		print(f'-- Address Register: 0x{result[2]:02X}')
	if ((result[3] != None) and (result[4] != None)):
		temp = result[4]+256*result[3]
		print(f'-- MUX Register: 0x{temp:04X}')
		print(f'-- MUX Register: {temp:016b}')
	if ((result[5] != None) and (result[6] != None)):
		temp = result[6]+256*result[5]
		print(f'-- DAC Register: {temp}')
	if ((result[7] != None) and (result[8] != None)):
		temp = result[8]+256*result[7]
		print(f'-- DIV Register: {temp}')
#----------------------------------
def put_data(ser, func, start, data, idx, dsize=1):
	if idx == 0:
		size = dsize
	else:
		size = len(data)
	if (DEBUG == 2):
		print(f'put_data Size: {size}')
	message = bytearray([func])
	message += start.to_bytes(3,'big')
	message += size.to_bytes(3,'big')
	message += idx.to_bytes(1,'big')
	message += data
	cs = make_checksum(message)
	message += bytearray((cs,))
	if (DEBUG == 2):
		dump_data(message,16)
	write(ser, message)
	expect_ok(ser)
	expect_done(ser)
#------------------------------------------
def get_data(ser, func, start, resp, idx):
	#print(f'------ big get ---------')
	message = bytearray([func])
	message += start.to_bytes(3,'big')
	message += resp.to_bytes(3,'big')
	message += idx.to_bytes(1,'big')
	#message += bytearray.fromhex('00') # just a dummy byte
	cs = make_checksum(message)
	message += bytearray((cs,))
	if (DEBUG == 2):
		dump_data(message,16)
	write(ser, message)
	expect_ok(ser)
	return read_with_checksum(ser, resp)
#------------------------------------------
def compare_data(data1, data2):
	for idx, i in enumerate (data1):
		if i != data2[idx]:
			return 0 # different
	return 1 # same
#------------------------------------------
def make_clockdata(ser, data):
		p0 = None
		p1 = None
		offset = None
		address = None
		mux = None
		dac = None
		div = None
		f0 = None
		f1 = None
		if (ser != None):
			clocksettings = get_data(ser, CLOCKR, 0, 9, 0)
		else:
			clocksettings = [17, 17, 8, 0, 78, 0, 0, 3, 43] # just dummy values for testing
		def_offset = clocksettings[0]   # first byte is range register
		o_offset = clocksettings[1]
		o_address = clocksettings[2]
		o_mux = clocksettings[4]+256*clocksettings[3]
		o_dac = clocksettings[6]+256*clocksettings[5]
		o_div = clocksettings[8]+256*clocksettings[7]
		newval = [None] * 9
		#print(data)
		if (data[0] == 'freq'):
			f0 = data[1]
			f1 = data[2]
		elif (data[0] == 'p0'):
			p0 = data[1]
		elif (data[0] == 'p1'):
			p1 = data[1]
		if (data[0] == 'off'):
			offset = data[1]
		if (data[0] != 'save'):
			address = 8
		if (data[0] == 'mux'):
			mux = data[1]
		if (data[0] == 'dac'):
			dac = data[1]
		if (data[0] == 'div'):
			div = data[1]

		if f0 != None and f1 != None:
			tf0 = f0
			tf1 = f1
			print(f'Calculating Values: {tf0}, {tf1}')
			offset,p0,o_dac,o_div = find_registers(tf0, tf1)
		if p0 != None:
			temp = o_mux & 0x1E7 # clr bit 3 and 4 (and 9)
			if p0 == 1:
				o_mux = temp 
			elif p0 == 2:
				o_mux = temp | 0x0008
			elif p0 == 4:
				o_mux = temp | 0x0010
			elif p0 == 8:
				o_mux = temp | 0x0018
		o_mux = o_mux & 0x1F9 # Set Prescaler 1 to 1
		if p1 != None:
			temp = o_mux & 0x1F9 # clr bit 1 and 2 (and 9)
			if p1 == 1:
				o_mux = temp 
			elif p1 == 2:
				o_mux = temp | 0x0002
			elif p1 == 4:
				o_mux = temp | 0x0004
			elif p1 == 8:
				o_mux = temp | 0x0006
		if offset != None:
			o_offset = offset + def_offset
		o_address = 8 # Disable automatic save (WC Bit = 1)
		if address != None:
			o_address = address
		if mux != None:
			o_mux = mux
		if dac != None:
			o_dac = dac
		if div != None:
			temp = o_mux & 0x1FE # clr bit 0 (and 9)
			if div == 1:
				o_mux = temp | 1 # set bit 0 (DIV1)
			else:
				o_mux = temp # bit 0 cleared
				o_div = div - 2
		newval[0] = def_offset # not used for writing
		newval[1] = o_offset
		newval[2] = o_address
		temp = o_mux.to_bytes(2, 'big')
		#print(f'MUX: {temp[0]}, {temp[1]}')
		newval[3] = temp[0]  #Hi
		newval[4] = temp[1]  #Lo
		temp = o_dac.to_bytes(2, 'big')
		newval[5] = temp[0]  #Hi
		newval[6] = temp[1]  #Lo
		temp = o_div.to_bytes(2, 'big')
		newval[7] = temp[0]  #Hi
		newval[8] = temp[1]  #Lo
		#print(f'------ New settings ------')
		#dump_registers(newval)
		return bytes(newval)	
#------------------------------------------
def dump_memorymap(mmap, cf):
	print(f'Memory Map:')
	print(f' Start    End      Chipsel. Per.')	
	for a in mmap:
		cspos = (~(mmap[a]))&0xFFFF
		if cspos == 0x8000:
			per = 'RAM'
		elif cspos == 0xC000:
			per = 'ROM'
		elif cspos == 0xFFFF:
			per = 'hole'
		else:
			for name in cf['peripherals'].keys():
				if name not in 'ram' and name not in 'rom':
					if (DEBUG == 3):
						print(f'Found other peripheral: {name}')
			per = name + ' (CS' + str(int(log(cspos, 2))) + ')'
		if a % 2 == 0: # even = start
			start = a
		else:
			print(f' 0x{start:06X}-0x{a:06X} 0x{cspos:04X}   {per}')	
#------------------------------------------
def config_per(cf): 
	new_file = bytearray((RAMSIZE*2) * b'\xFF')
	#dump_data(new_file, 16)
	keys = len(cf['peripherals'].keys())
	print(f'{bcolors.OKGREEN}------------------- Configure Peripherals ----------------------{bcolors.ENDC}')
	if (DEBUG == 3):
		print(f'found {keys} keys.')
	csdata = {}
	for i in range(keys):           # go through keys (like [[ram]])
		hi = 0
		low = 0
		name = cf['peripherals'].keys()[i]
		if (DEBUG == 3):
			print(f'Found peripheral: {name}')
		if (name in 'ram'): # take care of the chipselect for RAM and ROM
			cs = (~(1 << 15))&0xFFFF
			if (DEBUG == 3):
				print(f'    ram found, adding cs 15 0x{cs:04X}')
		elif (name in 'rom'):
			cs = (~((1 << 15) | (1 << 14)))&0xFFFF
			if (DEBUG == 3):
				print(f'    rom found, adding cs 14+15 0x{cs:04X}')
		else:
			if ('cs' in cf['peripherals'][name]):
				val = int(cf['peripherals'][name]['cs'],0)
				if (val >= 0 and val <14):
					cs = (~(1 << val))&0xFFFF
					if (DEBUG == 3):
						print(f'    chipselect - OK. {val} - 0x{cs:04X}')
				else:
					print(f'{bcolors.FAIL}wrong chipselect range  in {name} - {val}{bcolors.ENDC}')
					exit()
		for sk in cf['peripherals'][name]: # go through the values
			val = int(cf['peripherals'][name][sk],0)
			if ('cs' in sk): # cs already handled
				pass
			elif (('start' in sk) and ((val % 2) == 0)):   # test value (must be even) 
				low = val
				if (DEBUG == 3):
					print(f'    first Value even? - OK.')
				else:
					pass
			elif (('end' in sk) and ((val % 2) == 1)):     # test value (must be odd)
				high = val
				if (DEBUG == 3):
					print(f'    second Value odd? - OK. Adding from 0x{low:04X} - 0x{high:04X} w. cs 0x{cs:04X}')
				else:
					pass
				#print(f'    Adding {name} from 0x{low:04X} - 0x{high:04X} w. cs 0x{cs:04X}')
				csdata[low]=cs
				csdata[high]=cs
			else:
				print(f'{bcolors.FAIL}found wrong address (odd/even) in {name} at: {val:05X}{bcolors.ENDC}')
				exit()
	#print(f'Map1')
	#dump_memorymap(csdata, cf)
	sorted_dict = {k: csdata[k] for k in sorted(csdata)}
	#print(f'Map2')
	#dump_memorymap(sorted_dict, cf)
	# This adds all the holes to the memory Map
	sorted_dict2 = {}
	end = 0
	for i,k in enumerate(sorted_dict):
		#print(f'Key: {k:04X}, index: {i:04X}')
		if ((k != 0) and (i == 0)):      # first key is 0x0000 ?
			sorted_dict2.update({0:0})   # start at 0x0000
			sorted_dict2.update({k-1:0}) # until the first key
			#print(f'First key NOT 0!')
		if ((k-end != 1) and (i%2 == 0) and (i != 0)):   # start
			sorted_dict2.update({end+1:0})   # start at old end+1
			sorted_dict2.update({k-1:0})
			#print(f'hole! {k-end}, 0x{k:04X}')
		if i % 2 == 1: #odd = end
			end = k
		sorted_dict2.update({k:sorted_dict[k]})
	#print(f'Map3')
	dump_memorymap(sorted_dict2, cf)
	return dict_to_bytearray(sorted_dict2)
#------------------------------------------
def write_ram(ser, start, data,):
	size = len(data)
	if (DEBUG == 4):
		print(f'Size: {size:06X}, Start: {start:06X}')
	chunks = ceil(size / BLOCKSIZE) # how many chunks are needed?
	cstart = 0 # first chunk starts at start 
	cend = cstart + BLOCKSIZE
	crest = size - BLOCKSIZE
	if (DEBUG == 4):
		print(f'Making {chunks} chunks with each {BLOCKSIZE:.2f} bytes or {BLOCKSIZE} bytes.')
	for i in range(chunks):
		if (DEBUG == 4):
			length = len(data[cstart:cend])
			print(f'Sending chunk {i} from {cstart} to {cend} size: 0x{(length):04X} or {(cend-cstart)}')
		put_data(ser, RAMW, start, data[cstart:cend], i, size)
		if (chunks > 1) and (DEBUG == 0):
			val = int(100 * (i / chunks))
			print(f'Progress: {val:02d}%', end = '\r', file=sys.stdout, flush=True)
		cstart = cend
		crest -= BLOCKSIZE
		if (crest >= 0 ):
			cend += BLOCKSIZE
			if (DEBUG == 4):
				print(f'Rest: {crest} or 0x{crest:06X}, End: 0x{cend:06X}')
		else:
			cend = size
			if (DEBUG == 4):
				print(f'End: 0x{cend:06X} or {cend}')
	print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
#------------------------------------------
def read_ram(ser, start, size):
	retdata = bytearray()
	chunks = ceil(size / BLOCKSIZE) # how many chunks are needed?
	realchunksize = ceil(size / chunks) # we make evenly sized chunks
	cstart = start # first chunk starts at start 
	cend = cstart + realchunksize
	crest = size - realchunksize
	if (DEBUG == 4):
		print(f'Making {chunks} chunks with each {(size / chunks):.2f} bytes or {realchunksize} bytes.')
	for i in range(chunks):
		if (DEBUG == 4):
			print(f'Requesting chunk {i} from {cstart} to {cend} size: 0x{(cend-cstart):04X} or {(cend-cstart)}')
		retdata += get_data(ser, RAMR, cstart, (cend-cstart), i)
		if (chunks > 1) and (DEBUG == 0):
			val = int(100 * (i / chunks))
			print(f'Progress: {val:02d}%', end = '\r', file=sys.stdout, flush=True)
		cstart = cend
		crest -= realchunksize
		if (DEBUG == 4):
			print(f'Rest: {crest}')
		if (crest >= 0 ):
			cend += realchunksize
		else:
			cend = size+start
	print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
	return retdata
#------------------------------------------
def upload_image(ser, fpath, cf, name):
	try:
		imgstart = int(cf[name]['start'], 0)
		imgend  = int(cf[name]['end'] , 0)
		imgnumber = fpath + '/' + cf[name]['file']
		size = imgend - imgstart + 1
		#print(f'Image {i}: {imgstart:04X} to {imgend:04X} (size: {size} bytes) file: {imgnumber}')
		print(f'trying to upload {name} - {imgnumber} from: {imgstart:04X} to {imgend:04X}')
		img = read_file(imgnumber)
		if len(img) != size:
			print(f'{bcolors.FAIL}      ###### Size is wrong! (Image: 0x{len(img):04X} Space: 0x{size:04X}) ######{bcolors.ENDC}')
			exit()
		write_ram(ser, imgstart, img)
		return 0
	except Exception as e:
		#print(f'Error in upload_image: {e}!')
		return 1
#------------------------------------------
def upload_patch(ser, cf, name):
	try:
		imgstart = int(cf[name]['address'], 0)
		datalist = cf[name]['data']
		try:
			val = int(datalist, 16)
			length = 1
			img = val.to_bytes(1, 'big')
		except:
			datastr = ','.join(datalist)
			datastr = datastr.split(',')
			length = len(datastr)
		print(f'Datalist: {datalist}, Length: {length}')
		if (length > 1):
			datastr = ','.join(datalist)
			mylist = []
			#print(f'read {name}: Address: 0x{imgstart:04X} Data: {datastr}')
			for b in datastr.split(','):
				if b.isalnum():
					n = int(b, 16)
					#print(f'Received: {b} interpreted as: {n:02X}')
					mylist.append(n)
				else:
					print(f'couldnt interpret: {b}')
			img = bytes(mylist)
		print(f'trying to apply {name} at: {imgstart:04X}, {img}')
		write_ram(ser, imgstart, img)
		return 0
	except Exception as e:
		#print(f'Error in upload_patch: {e}!')
		return 1
#----------------------------------
def dump_val(state, buf, pos, sdr=0):
    if sdr == None:
        sdr = 0
    print(f'State: {x_state(state).name}, Len: {len(buf)} Pos: 0x{pos:04X} SDR: 0x{sdr:04X} ', end = '')
    for idx,itm in enumerate(buf):
        print(f'0x{itm:02X} ', end = '')
    print('')
#------------------------------------------
def xsvf_parser(ser, f):
    state  = x_state.XIDLE
    start = stop = 0
    length = 0
    sdr_bytes = 0
    for idx,itm in enumerate(f):
    #for idx in progressbar(range(len(f)), "Computing: ", 40):
        #itm = f[idx]
        #print(f'Byte: 0x{itm:02X} @ pos: {idx}')
        if (state != x_state.XIDLE):
            if (idx == stop):
                if state == x_state.XSDRSIZE:
                    sdr_bytes = 0
                    sdr_bytes |= f[stop-1]<<8
                    #print(f'Val1: 0x{f[stop-1]}, sdr: 0x{sdr_bytes:04X}')
                    sdr_bytes |= f[stop]
                    #print(f'Val2: 0x{f[stop]}, sdr: 0x{sdr_bytes:04X}')
                    sdr_bytes = (sdr_bytes + 7)>>3
                    #print(f'sdr: 0x{idx:02X}, 0x{start:02X}, 0x{stop:02X}')
                    #print(f'sdr: 0x{sdr_bytes:04X}')

                if state == x_state.XSIR2:
                    start = idx + 1
                    size = ((f[idx]+7)>>3)
                    stop = idx + size
                    print(f'sir2: 0x{idx:02X}, 0x{start:02X}, 0x{stop:02X} Size: {size}')
                    state = x_state.XSIR
                else:
                    #print(f'Dumping: {start} - {stop}, {f[start:stop+1]}')
                    dump_val(state, f[start:stop+1], start, sdr_bytes)
                    state = x_state.XIDLE
                    #print(f'State: {x_state(state).name}')
        else:
            if itm == 0:
                state = x_state.XCOMPLETE
                length = 0
            elif itm == 1:
                state = x_state.XTDOMASK
                length = sdr_bytes
            elif itm == 2:
                state = x_state.XSIR2
                length = 1
            elif itm == 3:
                state = x_state.XSDR
                length = sdr_bytes
            elif itm == 4:
                state = x_state.XRUNTEST
                length = 4
            elif itm == 7:
                state = x_state.XREPEAT
                length = 1
            elif itm == 8:
                state = x_state.XSDRSIZE
                length = 4
            elif itm == 9:
                state = x_state.XSDRTDO
                length = sdr_bytes*2
            elif itm == 0x0A:
                state = x_state.XSETSDRMASKS
                length = sdr_bytes*2
            elif itm == 0x0C:
                state = x_state.XSDRB
                length = sdr_bytes
            elif itm == 0x0D:
                state = x_state.XSDRC
                length = sdr_bytes
            elif itm == 0x0E:
                state = x_state.XSDRE
                length = sdr_bytes
            elif itm == 0x0F:
                state = x_state.XSDRTDOB
                length = sdr_bytes*2
            elif itm == 0x10:
                state = x_state.XSDRTDOC
                length = sdr_bytes*2
            elif itm == 0x11:
                state = x_state.XSDRTDOE
                length = sdr_bytes*2
            elif itm == 0x12:
                state = x_state.XSTATE
                length = 1
            else:
                print(f'Unrecogized Command: 0x{itm:02X}')
            start = idx + 1
            stop = idx + length
#------------------------------------------
def write_xsvf(ser, data):
	print(f'Writing XSVF')
	xsvf_parser(ser, data)

############################################
def main(ser, func, data = 0, start = 0, size = 1):

	if func == 'version':
		result = get_data(ser, VERSION, 0, 5, 0) #125 is for version read, 5 bytes response
		product = (result[0:3]).decode("utf-8")
		#print(f'Product: {product}')
		if (product == 'UC3'):
			print(f'Unicomp3')
			print(f'Version Major: {result[3]:02d}')
			print(f'Version Minor: {result[4]:02d}')
		else:
			print(f'different product!')		

	elif func == 'ramw':
		write_ram(ser, start, data)

	elif func == 'ramr':
		return read_ram(ser, start, size)

	elif func == 'ramv':
		print(f'Verifies RAM.')
		size = len(data)
		rbdata = read_ram(ser, start, size)
		if compare_data(data,rbdata):
			print(f'{bcolors.OKGREEN}Data matched!{bcolors.ENDC}')
		else:
			print(f'{bcolors.FAIL}Data different!{bcolors.ENDC}')

	elif func == 'config':
		cf = data
		fpath = start
		appname = cf['app']['name']
		version = cf['app']['ver']
		computername = cf['computer']['name']
		clockfreqf = int(cf['computer']['freqf'])
		try:
			clockfreqs = int(cf['computer']['freqs'])
		except:
			clockfreqs = 153600
			print(f'Using default Value for SlowClock: {clockfreqs}Hz')
		try:
			clocktic = float(cf['computer']['tic'])
		except:
			clocktic = 2.0
			print(f'Using default Value for TIC: {clocktic}Hz')
		print(f'Appname: {appname}, Version: {version}')
		print(f'Configure for:\n\t{computername} \n\t{clockfreqf/1E6:#.6f} MHz fast clock, \n\t{clockfreqs/1E6:#.6f} MHz slow clock,\n\t{clocktic:#.2f} Hz TIC.')		
		print(f'{bcolors.OKGREEN}------------------------ Reset Unicomp -------------------------{bcolors.ENDC}')
		put_data(ser, TRESETW, 0, bytearray.fromhex('01'), 0)  # Reset active
		print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
		retval = config_per(cf)
		if (DEBUG == 3):
			dump_data(retval, 16)
		print(f'{bcolors.OKGREEN}------------------------ Upload Config -------------------------{bcolors.ENDC}')
		put_data(ser, CONFIGW, 0, retval, 0, len(retval))
		print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
		print(f'{bcolors.OKGREEN}----------------------- Configure Clock ------------------------{bcolors.ENDC}')
		freqdata = []
		freqdata.append('freq')
		freqdata.append(clockfreqf*8) # fast clock is divided by eight in CPLD
		freqdata.append(clockfreqs)
		clockval = make_clockdata(ser, freqdata)
		if (DEBUG == 1):
			print(freqdata, clockval)
		put_data(ser, CLOCKW, 0, clockval, 0, len(clockval))
		print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
		print(f'{bcolors.OKGREEN}------------------------- Configure Tic ------------------------{bcolors.ENDC}')
		per = int(10000 / clocktic)
		print(f'Writing period of: {per}')
		put_data(ser, TICW, 0, per.to_bytes(2, 'big'), 0, 2)
		print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
		print(f'{bcolors.OKGREEN}----------------------- Upload ROM Image -----------------------{bcolors.ENDC}')
		for k in cf.keys():
			if 'img' in k:
				upload_image(ser, fpath, cf, k)
		print(f'{bcolors.OKGREEN}----------------------- Upload ROM Patches ---------------------{bcolors.ENDC}')
		for k in cf.keys():
			if 'patch' in k:
				#print(f'{k}')
				upload_patch(ser, cf, k)
		print(f'{bcolors.OKGREEN}------------------------ Reset inactive ------------------------{bcolors.ENDC}')
		put_data(ser, TRESETW, 0, bytearray.fromhex('00'), 0) # Reset inactive - Run
		print(f'{bcolors.OKGREEN}Done.        {bcolors.ENDC}')
		print(f'{bcolors.OKGREEN}------------------------ Modifications  ------------------------{bcolors.ENDC}')
		try:
			text1 = cf['modifications']['text']
			print(f'{bcolors.WARNING}{text1}{bcolors.ENDC}')
		except:
			print(f'{bcolors.WARNING}none{bcolors.ENDC}')
		exit()

	elif func == 'clockx':
		#print(data)
		clockval = make_clockdata(ser, data)
		put_data(ser, CLOCKW, 0, clockval, 0)

	elif func == 'clockr':
		clocksettings = get_data(ser, CLOCKR, 0, 9, 0)
		dump_registers(clocksettings)

	elif func == 'reset':
		put_data(ser, TRESETW, 0, data, 0)

	elif func == 'xsvf':
		write_xsvf(ser, data)

	elif func == 'tic':
		put_data(ser, TICW, 0, data, 0)
#------------------------------------------
def extract_files(img): # needed for .ucb files
	start = int.from_bytes(img[0:3], 'big', signed=False)
	imglen = int.from_bytes(img[3:6], 'big', signed=False)
	if ((start == 0xFFFFFF) and (imglen == 0)): # comment
		#comment = (img[6:]).encode('ascii',errors='ignore')
		comment = (img[6:]).decode('utf8')
		print(f'{bcolors.OKGREEN}{comment}{bcolors.ENDC}')
		return 0,0,0,''
	else:
		end = start + imglen
		ret = img[6:imglen+7]
		rest = img[imglen+7:]
		return start,end,ret,rest
#------------------------------------------
def get_value_with_kM(data): # converts input with kHz or Mhz to Hz
	value = None
	if (b[-1] == 'M'):
		if b[:-1].isalnum():
			value = 1000000 * int(b[:-1],0)
	elif(b[-1] == 'k'):
		if b[:-1].isalnum():
			value = 1000 * int(b[:-1],0)
	else:
		if b.isalnum():
			value = int(b,0)
	return value
############################################
#                 START
############################################
if __name__ == '__main__':
	parser = argparse.ArgumentParser(description = 'Upload script for the Unicomp3 RAMROM Board. Version: ' + str(ver))
	parser.add_argument(
		'-v', '--verbose',
		action = 'count',
		help = 'increase logging level')
	subparsers = parser.add_subparsers(
		dest = 'command',
		title = 'Operations',
		description = '(See "%(prog)s COMMAND -h" for more info)',
		help = '')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'ramw',
		help = 'write RAM data to device')
	subparser.add_argument(
		'start',
		nargs = '?',
		default = 0,
		help = 'memory start address')
	subparser.add_argument(
		'file',
		#nargs = '?',
		#default = sys.stdin.buffer,
		help = 'input filename or data in HEX (ex.: UC3_upload.py ramw :aa,0b,cc,... 0xF000) with no spaces!')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'ramv',
		help = 'compares RAM data to file')
	subparser.add_argument(
		'start',
		nargs = '?',
		default = 0,
		help = 'memory start address')
	subparser.add_argument(
		'file',
		#nargs = '?',
		#default = sys.stdin.buffer,
		help = 'filename to compare.')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'ramr',
		help = 'read RAM from device')
	subparser.add_argument(
		'start',
		help = 'start address (inclusive)')
	subparser.add_argument(
		'size',
		help = 'bytecount or with a dot in front until (.0xFF means start to 0xFF)')
	subparser.add_argument(
		'file',
		nargs = '?',
		#type = _parse_output_file,
		default = '', #sys.stdout.buffer,
		help = 'output filename [default: use stdout]')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'config',
		help = 'writes configuration data to device. A directory with all the data needs to be supplied.')
	subparser.add_argument(
		'file',
		help = 'config directory')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'clock',
		help = 'reads/writes clock data')
	subparser.add_argument(
		'function',
		help = '[freq]: frequency for Output 0,1 in Hz, kHz or MHz (comma separated), [div], [dac], [off]: R/W MUX, DIV, DAC, OFFSET Register, [save]: save settings to EPROM, [p0], [p1]: Prescaler for Output 0,1, [read] to read all Registers')
	subparser.add_argument(
		'data',
		#type = float,
		default = '',
		nargs = '?',
		help = 'input data (comma-separated no spaces!, single value or optional)')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'reset',
		help = 'activates Target Reset (0-Reset inactive, 1-Reset active, 2-Reset active for 10ms)')
	subparser.add_argument(
		'data',
		help = 'input 8-bit data')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'xsvf',
		help = 'writes .xsvf file to Target')
	subparser.add_argument(
		'file',
		help = 'xsvf file')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'tic',
		help = 'configures the slow tic timer')
	subparser.add_argument(
		'data',
		type = float,
		help = 'frequency in Hz')
	#---------------------------------------------------------------------------------    
	subparser = subparsers.add_parser(
		'version',
		help = 'display device version number and exit',
		add_help = False)
	#---------------------------------------------------------------------------------    

	args = parser.parse_args()
	if args.command == '':
		print("No command")
		exit()
	ser = None
	try:
		ser = Serial(port, 921600, timeout = 3, writeTimeout = 1)
	except IOError:
		print(f'{bcolors.FAIL}############################################{bcolors.ENDC}')
		print(f'{bcolors.FAIL}##              Port not found !          ##{bcolors.ENDC}')
		print(f'{bcolors.FAIL}##        working in simulation mode !    ##{bcolors.ENDC}')
		print(f'{bcolors.FAIL}##            nothing is uploaded !       ##{bcolors.ENDC}')
		print(f'{bcolors.FAIL}############################################{bcolors.ENDC}')
		#exit()
	#print(ser)
	if args.command == 'version':
		main(ser, 'version')

	elif args.command == 'config':
		arg = args.file
		if arg[-1] == '/':
			arg = arg[:-1]
		configdir = (arg.split("/"))[-1];
		configpath = arg
		configfile = configpath + '/' + configdir + '.cfg';
		print(f'{bcolors.OKCYAN}Configfile found: {configfile}{bcolors.ENDC}')
		cf = ConfigObj(configfile)
		main(ser, 'config', cf, configpath)

	elif args.command == 'reset':
		#mylist = []
		#b = args.data[0:]
		#n = int(b,0)
		n = int(args.data, 0)
		#mylist.append(n)
		#if (DEBUG == 5):
		#	print(f'Received: {b} interpreted as: {n:02X}')
		if (n not in (0,1,2)):
			print(f'Non-accepted value!')
			exit()
		#main(ser, 'reset', bytes(mylist))
		main(ser, 'reset', n.to_bytes(1, 'big'))

	elif args.command == 'clock':
		val = None	
		mylist = []
		if args.function == 'read': # Dump all Registers
			print(f'Read Registers')
			main(ser, 'clockr')
		elif  args.function == 'write': # Write all registers raw (not tested!)
			print(f'Not Implemented!')
			exit()
		else:
			if (args.function in ('freq', 'mux', 'dac', 'div', 'p0', 'p1', 'off', 'save')):
				func = args.function
				mylist.append(func)
				if ((args.data) == ''):
					print("A value is needed!")
					exit()
				for b in (args.data).split(','):
					val = get_value_with_kM(b)
					mylist.append(val)
					if (DEBUG == 1):
						print(f'Data: {b}, {val}')
				if (func == 'p0' or func == 'p1'):
					if (val not in (1,2,4,8)):
						print(f'Prescaler {val} not supported')
						exit()
				elif (func == 'off'):
					if (val < -7 or val > 6):
						print(f'Offset {val} not supported')
						exit()
				elif (func == 'div'):
					if (val < 2 or val > 1024):
						print(f'Divisor {val} not supported')
						exit()
				if (func == 'freq'):
					if (len(mylist) < 3):
						print(f'both frequencies have to be supplied!')
						exit()
					if ((mylist[1] > 66555000) or (mylist[2] > 66555000)):
						print("frequency too big, out of Range!")
						exit()
				main(ser, 'clockx', mylist)
			else:
				print(f'No valid Command given')
				exit()
	
	elif args.command == 'ramw':
		if args.file[0] == ':':    # Some bytes received (format :yy,xx,zz)
			#mylist = [int(e, 16) if e.isalnum() else e for e in (args.file[1:]).split(',')]
			mylist = []
			for b in (args.file[1:]).split(','):
				if b.isalnum():
					n = int(b, 16)
					if (DEBUG == 4):
						print(f'Received: {b} interpreted as: {n:02X}')
					mylist.append(n)
				else:
					print(f'couldnt interpret: {b}')

			img = bytes(mylist)
			start = int(args.start, 0) & (RAMSIZE-1)
			end = start + len(img) - 1
			if end > RAMSIZE:
				print(f'Write goes beyond {(RAMSIZE)}! (size: 0x{len(img):04X})')
				exit()
			main(ser, 'ramw', img, start)
		else:                   # real file received
			print(f'real file received {args.file}')
			img = read_file(args.file)
			ext = args.file.split(".")[-1] # check extension
			if ext != 'ucb': 
				start = int(args.start, 0)
				end = start + len(img) - 1
				if end > RAMSIZE:
					print(f'Write goes beyond {(RAMSIZE)}! Start: {start}, End: {end} Size: 0x{len(img):04X}')
					exit()
				main(ser, 'ramw', img, start)	
			else:                          # .ucb file has startaddress and size within
				print(f'Extension: {ext}')

				while True:
					start,end,oimg,rest = extract_files(img) # ucb file can contain more than one image
					if len(rest) == 0:
						break
					print(f'start: 0x{start:04X} end: 0x{end:04X}')
					main(ser, 'ramw', oimg, start)
					img = rest

	elif args.command == 'ramr':
		start = int(args.start, 0)

		sizetemp = args.size
		if sizetemp[0] == '.':
			endtemp = int(sizetemp[1:], 0)
			#print(f'sizetemp: {sizetemp[1:]}, endtemp: {endtemp}')
			size = endtemp - start + 1
			print(f'{bcolors.OKCYAN}Using endvalue. Calculated size is: {size:04X} or {size}{bcolors.ENDC}')
		else:
			size = int(sizetemp, 0)
			print(f'{bcolors.OKCYAN}Using size. Size is: {size:04X} or {size}{bcolors.ENDC}')
		if (size < 1) or (size > RAMSIZE):
			print(f'Size must be from 1 to {RAMSIZE} or 0x{(RAMSIZE):06X}!')
			exit()
		end = start + size - 1
		if end > RAMSIZE:
			print(f'Read goes beyond 0x{(RAMSIZE):06X}!')
			exit()
		if (DEBUG == 4):
			print(f'Start: 0x{start:06X}, End: 0x{end:06X}, Size: 0x{size:06X}')
		data = main(ser, 'ramr', 0, start, size)
		write_file(data, args.file)

	elif args.command == 'ramv':
		img = read_file(args.file)
		main(ser, 'ramv', img, int(args.start, 0))

	elif args.command == 'xsvf':
		img = read_file(args.file)
		ext = args.file.split(".")[-1] # check extension
		if ext != 'xsvf':
			print(f'{bcolors.FAIL}Extension NOT xsvf!{bcolors.ENDC}')
			exit()
		main(ser, 'xsvf', img)

	elif args.command == 'tic':
		n = float(args.data)
		per = int(10000 / n)
		if (DEBUG == 6):
			print(f'Data: {n}, Period: {per}')
		main(ser, 'tic', per.to_bytes(2, 'big'))
	try:
		ser.close()
	except:
		pass
