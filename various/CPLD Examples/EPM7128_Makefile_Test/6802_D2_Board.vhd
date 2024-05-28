library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity cpldD2 is
    Port ( vma             : in std_logic;
              rnw             : in std_logic;
              nreset            : in std_logic;
              nrd             : out std_logic;
              nwr             : out std_logic;             
              address         : in std_logic_vector(15 downto 0);
              csbar             : out std_logic_vector(4 downto 0);
              CLKIN         : in  STD_LOGIC; -- MClock pin 70 (3.6Mhz)
              Data_out        : inout std_logic_vector(7 downto 0);
           clockout2    : out STD_LOGIC; -- divided MClock/2 -->1.8MHz
           clockout4    : out STD_LOGIC);-- divided MClock/4 clock -->0.9MHz

end cpldD2;


architecture Behavioral of cpldD2 is


signal clockTemp : std_logic_vector (2 downto 0);

begin

    -- clock divider
    process (CLKIN)
    begin
        if (CLKIN'Event and CLKIN = '1') then
            clockTemp <= clockTemp + '1';
        end if;
    end process;

    clockout2 <= clockTemp(0); -- divided by 2 clock
    clockout4 <= clockTemp(1); -- divided by 4 clock

-- set D0-D7 @ high Impedance TRISTATE
Data_out <= "ZZZZZZZZ";


-- CS NVRAM 0000-03FFF,A000-A0FF as RAM E000-EFFF,FC00-FFFF as ROM
    csbar(0) <= '0' when
        ((vma = '1') and (rnw ='1') and (nreset= '1') and
        (((address >= X"E000") and (address <= X"E3FF")) or ((address >= X"FC00") and (address <= X"FFFF"))))
        or
        ((vma = '1') and (nreset= '1') and
        ((address >= X"0000") and (address <= X"3FFF")))
        or
        ((vma = '1') and (nreset= '1') and
        ((address >= X"A000") and (address <= X"A0FF")))       

        else '1';

-- CS ACIA TAPE 8008-800F
    csbar(1) <= '0' when
        ((vma = '1')and (nreset= '1') and
        ((address >= X"8008") and (address <= X"800F")))
        else '1';
       
-- CS 2nd ACIA 8010-801F
    csbar(2) <= '0' when
        ((vma = '1') and (nreset= '1') and
        ((address >= X"8010") and (address <= X"801F")))
        else '1';
       
-- Keypad PIA 8020-8024       
    csbar(3) <= '0' when
        ((vma = '1') and (nreset= '1') and
        ((address >= X"8020") and (address <= X"8024")))
        else '1';
-- 2nd PIA 8000-8007           
    csbar(4) <= '0' when
        ((vma = '1') and (nreset= '1') and
        ((address >= X"8000") and (address <= X"8007")))
        else '1';

-- Seperate NREAD and NWRITE signals from rnw
        nrd <= not (rnw and vma);
        nwr <= not((not(rnw)) and vma);

end Behavioral;