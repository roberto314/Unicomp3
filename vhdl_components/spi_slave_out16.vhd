library ieee; 
use ieee.std_logic_1164.all; 

entity spi_slave_out16 is 
    port(
        sclk : in std_logic;
        mosi : in std_logic;
        load : in std_logic; 
        data : in std_logic_vector(15 downto 0); 
        miso : out std_logic
        ); 
end spi_slave_out16; 

architecture archi of spi_slave_out16 is
  signal tmp: std_logic_vector(15 downto 0);
  begin
    process (sclk)
      begin
        if (sclk'event and sclk='1') then
          if (load='1') then
            tmp <= data;
          else
            tmp <= tmp(14 downto 0) & mosi;
          end if;
        end if;
    end process;
    miso <= tmp(15);
end archi;