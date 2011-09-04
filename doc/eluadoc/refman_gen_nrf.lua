-- eLua reference manual - nRF module

data_en = 
{

  -- Title
  title = "eLua reference manual - nRF module",

  -- Menu name
  menu_name = "nrf",

  desc = "Radio interface (nRF24L01) operations",

  -- Overview
  overview = [[This module contains functions for sending and receiving data over the wireless nRF24L01-based interface.]],
  
  -- Structures
  structures =
  {
    { text = [[aa:bb:cc:dd:ee]],
      name = "Radio module address",
      desc = "The radio module address is specified in the format $aa:bb:cc:dd:ee$ where each part of the address is a 2-digit hexadecimal number.",
    }
  },

  -- Functions
  funcs = 
  {
    { sig = "#nrf.set_address#( address )",
      desc = "Sets the address of the radio module.",
      args = "$address$ - the address of the module."
    },

    { sig = "#nrf.set_mode#( mode )",
      desc = "Sets the radio interface in transmitter or receiver mode.",
      args = "$mode$ - $nrf.TX$ to set as transmitter, $nrf.RX$ to set as receiver."
    },

    { sig = "sentbytes = #nrf.write#( address, data )",
      desc = "Sends data to another nRF radio interface.",
      args = 
      {
        "$address$ - the address of the remote radio interface.",
        "$data$ - the data to send.",
      },
      ret = "The number of bytes actually sent."
    },

    { sig = "data = #nrf.read#( [timeout], [timer_id] )",
      desc = "Receives data from another nRF radio interface with optional timeout.",
      args =
      {
        [[$timeout (optional)$ - the receive timeout in microseconds. It can also be $nrf.NO_TIMEOUT$ (the function will return immediately) or $nrf.INF_TIMEOUT$ (the read function will block indefinitely until data is received). If the timeout isn't equal to either $nrf.NO_TIMEOUT$ or $nrf.INF_TIMEOUT$, the $timer_id$ argument must also be specified.]],
        [[$timer_id$ - the ID of the timer used for measuring the timeout.]]
      },
      ret = "The data received from the remote radio interface or an empty string if no data was received."
    }
  },
}

data_pt = data_en
