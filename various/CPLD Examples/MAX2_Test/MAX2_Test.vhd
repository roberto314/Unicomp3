library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity blinky is
    Port (  LED           : out std_logic;             
            clk50         : in  STD_LOGIC);
end blinky;


architecture Behavioral of blinky is

signal clockTemp : std_logic_vector (30 downto 0);

begin

    -- clock divider
    process (clk50)
    begin
        if (clk50'Event and clk50 = '1') then
            clockTemp <= clockTemp + '1';
        end if;
    end process;

    LED <= clockTemp(25);

end Behavioral;