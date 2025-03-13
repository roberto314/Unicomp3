library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

-- This is part of the UNICOMP Project
-- Robert Offner 2024
-- This file is the DC5 Floppy Controller (a CPLD Implementation of SWTPC DC-4 Controller)
-- Function:
-- Generate a Motor Timer with external 1s Tic Signal,
-- Gen. Head Load Timer (120ms)
-- Gen. a Write Register for Drive Select and Motor on 
-- Gen. a Read Register with two Bits (INTRQ and DRQ)
--
-- v0.1
--
-- Version History:
-- v0.1: Initial (does not work!)
--

entity dc5 is
    Port ( 
        D        : inout std_logic_vector(7 downto 0); -- Data Bus
        nNMI     : out std_logic; -- DEBUG OUTPUT
        nINT     : out std_logic; -- DEBUG OUTPUT
        nDSKRDY  : in  std_logic; -- 
        nDSEN    : out std_logic; -- Drive Select Enable
        DSB      : out std_logic; -- Drive Select A0
        DSA      : out std_logic; -- Drive Select A1
        nSIDE1   : out std_logic; -- 
        MOTCP    : out std_logic; -- Motor Enable Output (high active)
        nDSKCHG  : in  std_logic; -- 
        nINTRQ   : in  std_logic; -- 
        DRQ      : in  std_logic; -- 
        nDDEN    : out std_logic; -- 
        nWG      : in  std_logic; -- 
        MOTFDC   : in  std_logic; -- Motor Input from FDC
        CLK1     : in  std_logic; -- 1Hz Input 
        HLT      : out std_logic; -- HALT to FDC
        FDCCLK   : out std_logic; -- Output to FDC
        nCE0     : in  std_logic; -- Chip Enable
        RW       : in  std_logic; -- Read/Write
        E        : in  std_logic; -- E Clock from CPU
        A3       : in  std_logic; -- A3 Address Line
        nRST     : in  std_logic; -- Reset from CPU
        HLD      : in  std_logic; -- Head Load
        nDOE     : out std_logic; -- Output Enable of '245'
        CLK8M    : in  std_logic; -- 8MHz Clock Input
        SSO      : in  std_logic  -- Pin 25
        );
end dc5;

architecture Behavioral of dc5 is
    signal clk_divider  : unsigned(2 downto 0);
    signal motor_cnt    : unsigned(4 downto 0);
    signal headload_cnt : unsigned(19 downto 0);
    signal s_CS         : std_logic :='1'; -- Chip Select with E
    signal s_WP         : std_logic :='1'; -- Write Pulse Drive Register
    signal s_RP         : std_logic :='1'; -- Read Pulse Status Register
    signal s_MOT        : std_logic :='0'; -- Motor Flag
    signal s_HLDFLG     : std_logic :='0'; -- Head Load Flag
    signal s_HLT        : std_logic :='1'; -- Head Load Timing Signal
    signal s_MOTLCK     : std_logic :='0'; -- Motor Lock
    signal s_DSA        : std_logic :='0';
    signal s_DSB        : std_logic :='0';
    signal s_SIDE1      : std_logic :='0';
    signal s_CLKSTAT    : std_logic; -- Status Flag to get the edge of 1s Clock Signal
    signal s_DSEN       : std_logic :='1'; -- DriveSelect Enable (Low active)

begin

--FDCCLK <= clk_divider(1); -- divide by 4 for 2MHz
FDCCLK <= clk_divider(2); -- divide by 8 for 1MHz
s_CS <= nCE0 OR NOT E;
DSA <= s_DSA;
DSB <= s_DSB;
nSIDE1 <= s_SIDE1;
MOTCP <=  s_MOT;
nDSEN <= s_DSEN OR (NOT s_MOT);
nDDEN <= NOT SSO;
nNMI <= s_WP; -- DEBUG
nINT <= s_RP; -- DEBUG

s_RP <= (NOT RW) OR A3 OR s_CS; -- A3 is Low for Status Read
nDOE <= s_CS OR (NOT A3); -- A3 is high for WD2797
D <= nINTRQ&DRQ&"000000" when s_RP = '0' and nRST = '1' else (others => 'Z');

HLT <= s_HLT; 
--HLT <= '0'; --DEBUG

process (CLK8M, CLK1)
    begin
        if rising_edge(CLK8M) then
            if nRST = '0' then
                s_MOT <= '0';
                s_HLT <= '1';
            else
                clk_divider  <= clk_divider + 1; -- FDC Clock Output divider
---------------------------------------------------------- 1Hz Tic (External)
                if CLK1 = '1' and s_CLKSTAT = '0' then -- rising edge
                    s_CLKSTAT <= '1';
                    if motor_cnt > 0 then
                        motor_cnt <= motor_cnt - 1;
                    end if;
                end if;
                if CLK1 = '0' and s_CLKSTAT = '1' then -- falling edge
                    s_CLKSTAT <= '0';
                end if;
---------------------------------------------------------- Motor Timer
                if s_CS = '0' then
                    s_MOT <= '1';
                    motor_cnt <= "01111"; -- 15s
                end if;                
                if motor_cnt = 0 and s_MOT = '1' then
                    s_MOT <= '0';
                end if;
---------------------------------------------------------- Head Load Tmr.
                if HLD = '1' and s_HLDFLG = '0' then
                    s_HLT <= '0'; --NOT (HLD OR s_HLDFLG);
                    s_HLDFLG <= '1';
                    headload_cnt <= "11101010011000000000"; -- 960000 == 120ms
                end if;
                if HLD = '0' then
                    s_HLDFLG <= '0';
                end if;
                if headload_cnt = 0 and s_HLT = '0' then
                    s_HLT <= '1';
                end if;
                if headload_cnt > 0 then
                    headload_cnt <= headload_cnt - 1;
                end if;
            end if;
        end if;
    end process;

s_WP <= RW OR A3 OR nCE0; -- A3 is Low for Drive Register
process (E)
    begin
        if falling_edge(E) then
            if nRST = '0' then
                s_DSA <= '0';
                s_DSB <= '0';
                s_SIDE1 <= '0';
                s_DSEN <= '1';
                
            else
                if s_WP = '0' then -- 8014-8017
                    s_DSA   <= D(0);
                    s_DSB   <= D(1);
                    s_SIDE1 <= D(6);
                    s_DSEN  <= D(7);
                end if;
            end if;
        end if;
    end process;
end Behavioral;