library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity blinky is
    Port (  LED           : out std_logic;             
            CLKIN         : in  STD_LOGIC);
end blinky;


architecture Behavioral of blinky is

signal clockTemp : std_logic_vector (20 downto 0);

begin

    -- clock divider
    process (CLKIN)
    begin
        if (CLKIN'Event and CLKIN = '1') then
            clockTemp <= clockTemp + '1';
        end if;
    end process;

    LED <= clockTemp(18);

end Behavioral;