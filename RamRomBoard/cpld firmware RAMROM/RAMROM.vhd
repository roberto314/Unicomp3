library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;

-- This is part of the UNICOMP Project
-- Robert Offner 2024
-- This file is for the RAMROM Board.
-- Function:
-- 
-- 
-- v0.1
--
-- Version History:
-- v0.1: Initial (doesn't work - cpld too small)
--


entity RAMROM is
    Port ( 
        D        : in std_logic_vector(15 downto 0); --:= ( others => 'Z'); -- Data to/from RAM
        --D        : out std_logic_vector(15 downto 0) := "ZZZZZZZZZZZZZZZZ"; -- Data to/from RAM
        CLK      : in std_logic;   -- Main Clock Input (25MHz)
        nCPUFREE : in  STD_LOGIC;  -- Bus Free in low means High-Z on Bus (1MHz)
        SCK      : in  STD_LOGIC;  -- SPI Interface
        MISO     : out STD_LOGIC;  -- SPI Interface
        MOSI     : in  STD_LOGIC;  -- SPI Interface
        nMWR     : in STD_LOGIC;   -- Mem Write from Bus
        nMRD     : in STD_LOGIC;   -- Mem Read from Bus
        nDATRDY  : out STD_LOGIC;  -- Ready to STM32
        nRWREQ   : in  STD_LOGIC;    -- Read Request from STM32
        nUB      : out STD_LOGIC := '1'; -- upper Byte to RAM
        nLB      : out STD_LOGIC := '1'; -- lower Byte to RAM
        nBUSCLR  : out STD_LOGIC := '1'; -- Bus Free to BUS
        nRST     : in  STD_LOGIC;        -- Reset from Target
        nPLD     : in  STD_LOGIC;        -- Parallel Load from STM32
        nCPLDCS  : in  STD_LOGIC;        -- 
        RES1     : in  STD_LOGIC;        -- 
        nCNTOE   : out STD_LOGIC := '1';  -- Output Enable to Counter
        nDATEN   : out STD_LOGIC := '1'  -- Output Enable to Data Buffer
        );
end RAMROM;

architecture Behavioral of RAMROM is
    alias nRAMEN    is D(15);
    alias RAMPROT   is D(14);
    alias CESPARE   is D(13);
    alias nCEOUTEN  is D(12);
    alias CESEL3    is D(11);
    alias CESEL2    is D(10);
    alias CESEL1    is D(9);
    alias CESEL0    is D(8);

    --signal s_Din          : std_logic_vector(15 downto 0);
    --signal s_Dout         : std_logic_vector(15 downto 0);
    signal clk_divider  : unsigned(7 downto 0);
    signal s_Bus        : std_logic := '0';
    signal s_DatainProgress    : std_logic := '0';
    signal s_ReadinProgress    : std_logic := '0';
    signal s_BufferR    : std_logic;
    signal s_BufferW    : std_logic;
    signal s_Dir        : std_logic := '1';

begin

s_BufferR <= nMRD or nRAMEN; -- Read Enable 
s_BufferW <= nMWR or nRAMEN or RAMPROT; -- Write Enable
nDATEN <= s_BufferR and s_BufferW;
nLB <= s_BufferW;

--s_Din <= D  when s_Dir='1' else (others => 'Z');
--D <= s_Dout when s_Dir='0' else (others => 'Z');


   spi_out : entity work.spi_slave_out16 -- shift register
   port map (
      sclk     => SCK,
      ss_n     => nRWREQ,
      mosi     => '0',
      miso     => MISO,
      data     => D
   );

process (CLK, nCPLDCS)
    begin
        if rising_edge(CLK) then
            if (nCPUFREE = '1' and s_Bus = '0') then -- BUS rising edge
                s_Bus <= '1';
            end if;

            if (nCPUFREE = '0' and s_Bus = '1') then -- BUS falling edge
                s_Bus <= '0';
                clk_divider <= (others => '0');
            end if;
            
            if (nCPUFREE = '0' and s_Bus = '0') then -- BUS no clock

            end if;
            
            if (nCPLDCS = '1' and s_DatainProgress = '1') then -- CS rising edge
                s_DatainProgress <= '0'; -- SPI Data Ready or transfer finished
                if nRWREQ = '1' then -- Writing Data
                    --D <= s_Din;
                    clk_divider <= (others => '0');
                end if;
            end if;

            if (nCPLDCS = '0' and s_DatainProgress = '0') then -- CS falling edge
                s_DatainProgress <= '1'; -- Data Transfer over SPI in Progress
            end if;

            if (nRWREQ = '1' and s_ReadinProgress = '1') then -- RWREQ rising edge
                s_ReadinProgress <= '0'; -- Parallel Read Transfer finished
            end if;

            if (nRWREQ = '0' and s_ReadinProgress = '0') then -- RWREQ falling edge
                s_ReadinProgress <= '1'; -- Parallel Read Requested
                --s_Dout <= D;
                clk_divider <= (others => '0');
            end if;

            clk_divider <= clk_divider + 1;

            if clk_divider = x"01" then
                nDATRDY <= '0';
            end if;
            if clk_divider = x"06" then
                nDATRDY <= '1';
            end if;
            if clk_divider = x"FF" then
                --D <= (others => 'Z');
            end if;
        end if;
end process;

end Behavioral;
