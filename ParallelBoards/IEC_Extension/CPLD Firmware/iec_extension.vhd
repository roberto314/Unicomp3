library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

-- This is part of the UNICOMP Project
-- Robert Offner 2024
-- This file is for the IEC Extension Board.
-- Function:
-- This implementation is for a 1581 Drive
-- 
-- v0.1
--
-- Version History:
-- v0.1: Initial
--

entity iec_extension is
    Port ( 
        SCLK_in  : in std_logic; -- from IEC Conn.
        SDAT_in  : in std_logic; -- from IEC Conn.
        ATN_in   : in std_logic; -- from IEC Conn.
        FCLK_in  : in std_logic; -- from IEC Conn.
        RST_in   : in std_logic; -- from IEC Conn.
        SCLK_out : out std_logic := '0'; -- to IEC Conn.
        SDAT_out : out std_logic := '0'; -- to IEC Conn.
        ATN_out  : out std_logic := '0'; -- to IEC Conn.
        FCLK_out : out std_logic := '0'; -- to IEC Conn.
        RST_out  : out std_logic := '0'; -- to IEC Conn.

        ATN_drv   : in std_logic; -- from 8520
        SDAT_drv  : in std_logic; -- from 8520
        SCLK_drv  : in std_logic; -- from 8520
        ATN_sens  : out std_logic := '0'; -- to 8520
        SDAT_sens : out std_logic := '0'; -- to 8520
        SCLK_sens : out std_logic := '0'; -- to 8520
        );
end iec_extension;

architecture Behavioral of iec_extension is
    --signal w_n_CS       : std_logic :='1';

begin

end Behavioral;