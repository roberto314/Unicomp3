library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

-- This is part of the UNICOMP Project
-- Robert Offner 2024
-- This file is for the 6809 CPU Board.
-- Function:
-- Generate E and Q Outputs for 6809E CPU,
-- Gen. PHI0 and NOT PHI0 for second CPU,
-- Gen. RD and WR Signals,
-- 
-- v0.1
--
-- Version History:
-- v0.1: Initial
--

entity board_6809 is
    Port ( 
        A        : in std_logic_vector(15 downto 0);
        RES0     : in std_logic;
        RES1     : in std_logic;
        nDOE     : out STD_LOGIC := '1'; -- Data Buffer Enable
        nAOE     : out STD_LOGIC := '1'; -- Address Buffer Enable
        CLKF     : in std_logic;  -- Main Clock Input
        RnW      : in  STD_LOGIC; -- R/W from CPU
        E_C      : inout STD_LOGIC; -- E to CPU
        Q_C      : inout STD_LOGIC; -- Q to CPU
        --E_C      : in  STD_LOGIC; -- E from CPU
        --Q_C      : in  STD_LOGIC; -- Q from CPU
        CLKCPU   : out STD_LOGIC; -- Clock to CPU
        PH0      : out STD_LOGIC; -- PHI 0 to CPU
        nPH0     : out STD_LOGIC; -- PHI 0 to CPU (inverted)
        BA       : in  STD_LOGIC; -- Bus Access from CPU
        nRST     : in  STD_LOGIC; -- Reset CPU
        nBUSFREE : out  STD_LOGIC; -- Bus Free means High-Z on Bus
        nMWR     : out STD_LOGIC := '1'; -- Mem Write to Bus
        nMRD     : out STD_LOGIC := '1'  -- Mem Read to Bus
        );
end board_6809;

architecture Behavioral of board_6809 is
    --signal w_n_CS       : std_logic :='1';
    signal clk_divider  : unsigned(2 downto 0);
    signal s_nMRD       : std_logic :='1';
    signal s_nMWR       : std_logic :='1';
    signal s_BUS        : std_logic :='1';
    signal s_PH0        : std_logic :='1';
    signal n_PH0        : std_logic :='1';
    signal s_E          : std_logic :='1';
    signal s_Q          : std_logic :='1';
    signal s_ELEVEL     : std_logic :='1';

begin

process (CLKF)
    begin
        if rising_edge(CLKF) then
            clk_divider   <= clk_divider + 1;
            case clk_divider is
                when "000" =>  -- 0
                    s_E <= '1';
                    s_PH0 <= '1';
                    n_PH0 <= '0';
                when "001" =>  -- 1
                    s_PH0 <= '0';
                    n_PH0 <= '1';
                when "010" =>  -- 2
                    s_Q <= '0';
                    s_PH0 <= '1';
                    n_PH0 <= '0';
                when "011" =>  -- 3
                    s_PH0 <= '0';
                    n_PH0 <= '1';
                when "100" =>  -- 4
                    s_E <= '0';
                    s_PH0 <= '1';
                    n_PH0 <= '0';
                when "101" =>  -- 5
                    s_PH0 <= '0';
                    n_PH0 <= '1';
                when "110" =>  -- 6
                    s_Q <= '1';
                    s_PH0 <= '1';
                    n_PH0 <= '0';
                when "111" =>  -- 7
                    s_PH0 <= '0';
                    n_PH0 <= '1';
                when others =>
            end case;

            --if clk_divider = "111" then -- if 8 reset
            --    clk_divider <= "000";
            --end if;
            --if clk_divider = "000" then -- 0
            --    s_E <= '1';
            --end if;
            --if clk_divider = "010" then -- 2
            --    s_Q <= '0';
            --end if;
            --if clk_divider = "100" then -- 4
            --    s_E <= '0';
            --end if;
            --if clk_divider = "110" then -- 6
            --    s_Q <= '1';
            --end if;
        end if;
    end process;
    --s_PH0 <= clk_divider(0); -- divide by 2

nPH0 <= n_PH0;
PH0 <= s_PH0;
E_C <= s_E;
Q_C <= s_Q;
nMRD <= s_nMRD;
nMWR <= s_nMWR;
CLKCPU <= s_PH0;

-------------------- external RD and WR Signals ---------------
s_nMRD <= NOT(RnW AND s_E);       -- thats how it is usually done.
s_nMWR <= NOT((NOT RnW) AND s_E); -- thats how it is usually done.

-- nAOE <= '0'; -- works (2MHz possible)
s_BUS <= nRST AND (s_E); -- Reset must release bus bc. of STM32!

nAOE <= NOT(s_BUS); -- test -works
nDOE <= NOT(s_BUS); -- test -works
nBUSFREE <= s_BUS;

--process (CLKF, s_PH0) --this delays the PHI0 to the CPU for half period of CLKF (8MHz) -not needed!
--    begin
--        if falling_edge(CLKF) then
--            --if s_PH0 = '1' then
--                --nPH0 <= NOT(s_PH0);
--                --PH0 <= s_PH0;
--            --end if;
--        end if;
--    end process;
end Behavioral;