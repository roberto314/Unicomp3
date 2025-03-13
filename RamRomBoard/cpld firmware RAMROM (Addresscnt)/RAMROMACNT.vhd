library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

-- This is part of the UNICOMP Project
-- Robert Offner 2024
-- This file is a 20 bit Counter with 8-bit preload and Output enable
-- Function:
-- Bits 12..19 are preloaded via SPI, Bits 0..11 are Reset to zero
-- With every clock of ACLK address is incremented.
-- It is used as Addresscounter for the RAMROM Board.
-- v0.1
--
-- Version History:
-- v0.1: Initial
--

entity RAMROMACNT is
    Port ( 
        A        : out std_logic_vector(19 downto 0) := ( others => 'Z'); -- Address to RAM
        nCPUFREE : in  STD_LOGIC;  -- Bus Free in low means High-Z on Bus (1MHz)
        SCK      : in  STD_LOGIC;  -- SPI Interface
        MOSI     : in  STD_LOGIC;  -- SPI Interface
        nACLK    : in STD_LOGIC;   -- Clock for addresscounter
        nCNTCE  : in  STD_LOGIC;  -- Reset Counter when low and falling ACLK
        nCNTOE   : in STD_LOGIC    -- Input to Enable Counter Output
        );
end RAMROMACNT;

architecture Behavioral of RAMROMACNT is
    signal dat_reg    : std_logic_vector(7 downto 0); 
    signal s_ATEMP        : unsigned(19 downto 0) := (others => '0');

begin

process (SCK) -- Shift Register
    begin
        if rising_edge(SCK) then
            if nCNTCE = '0' then
                dat_reg <= dat_reg(6 downto 0) & MOSI;
            end if;
        end if;
end process;

process (nACLK, nCNTCE) -- Count up or Reset
    begin
        if falling_edge(nACLK) then
            if nCNTCE = '0' then
                s_ATEMP <= (others => '0');
            else
                s_ATEMP <= s_ATEMP + 1;
            end if;
        end if;
end process;

process (nCPUFREE, nCNTOE) -- If BUS is free output address
    begin
        if falling_edge(nCNTOE) then
            if (nCPUFREE = '0' and nCNTOE = '0') then -- BUS free
                A <= std_logic_vector(to_unsigned(to_integer(unsigned(dat_reg(7 downto 0) & "000000000000")),20) + s_ATEMP);
            else
                A <= ( others => 'Z');
            end if;
        end if;
end process;

end Behavioral;
