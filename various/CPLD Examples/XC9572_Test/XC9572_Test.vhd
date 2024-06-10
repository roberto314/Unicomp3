library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

entity XC9572_Test is
    Port ( 
        button     : in std_logic;
        CLK        : in std_logic;
        LED        : out  STD_LOGIC
        );
end XC9572_Test;

architecture Behavioral of XC9572_Test is
    signal clk_divider  : unsigned(21 downto 0);

begin

--LED <= NOT(button);
process (CLK)
    begin
        if rising_edge(CLK) then
            clk_divider   <= clk_divider + 1;
        end if;
    end process;
LED <= clk_divider(18);
end Behavioral;