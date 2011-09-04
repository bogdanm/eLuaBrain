-- eLua reference manual - net module

data_en = 
{

  -- Title
  title = "eLua reference manual - net module",

  -- Menu name
  menu_name = "net",

  desc = "TCP/IP and UDP networking",

  -- Overview
  overview = [[This module contains functions for accessing a TCP/IP network from eLua. It can be enabled only if networking support is enabled` (see @building.html@building@ for details).`.`]],

  -- Structures
  structures =
  {
    { text = [[// eLua net error codes
enum
{
  ELUA_NET_ERR_OK = 0,            // exported as $net.ERR_OK$
  ELUA_NET_ERR_TIMEDOUT,          // exported as $net.ERR_TIMEDOUT$
  ELUA_NET_ERR_CLOSED,            // exported as $net.ERR_CLOSED$
  ELUA_NET_ERR_ABORTED,           // exported as $net.ERR_ABORTED$
  ELUA_NET_ERR_OVERFLOW           // exported as $net.ERR_OVERFLOW$
};]],
      name = "Error codes",
      desc = "These are the error codes defined by the eLua networking layer. They are returned by a number of functions in this module.",
    }
  },

  -- Functions
  funcs = 
  {
    { sig = "ip = #net.packip#( [ip1, ip2, ip3, ip4] or strip )",
      desc = [[Returns an internal representation of an IP address that can be used with all function from the $net$ module that expect an IP address argument. If the first arguments of this function is an integer, the IP is considered to be in the format %ip1.ip2.ip3.ip4%. If the first (and only) argument of this function is a string, the IP address is parsed directly from its string representation.]],
      args = 
      {
        "$ip1, ip2, ip3, ip4$ - the components of the IP address, $OR$",
        "$strip$ - the IP address in string format."
      },
      ret = "An integer that encodes the IP address in an internal format."
    },

    { sig = "[ip1, ip2, ip3, ip4] or strip = #net.unpackip#( ip, format )",
      desc = "Returns an unpacked representation of an IP address encoded by @#net.packip@net.packip@.",
      args = 
      {
        "$ip$ - the encoded IP address.",
        "$format$ - the unpack format. This can be either $*n$ to unpack the IP address as 4 different integer values or $*s$ to unpack the IP address in string format.",
      },
      ret = 
      {
        "$ip1, ip2, ip3, ip4$ - the components of the IP address. This will be returned only if $format == '*n'$.",
        "$strip$ - the IP in string format. This will be returned only if $format == '*s'$.",
      }
    },
   
    { sig = "ip = #net.lookup#( hostname )",
      desc = "Does a DNS lookup.",
      args = "$hostname$ - the name of the computer.",
      ret = "The IP address of the computer."
    },

    { sig = "socket = #net.socket#( type )",
      desc = "Create a socket for TCP/IP communication.",
      args = [[$type$ - can be either $net.SOCK_STREAM$ for TCP sockets or $net.SOCK_DGRAM$ for UDP sockets.]],
      ret = "The socket that will be used in subsequent operations."
    },

    { sig = "res = #net.close#( socket )",
      desc = "Close a socket.",
      args = "$socket$ - the socket to close.",
      ret = "An error code`, as defined @#error_codes@here@.`.`"
    },
    
    { sig = "#net.set_buffer#( sock, bufsize )",
      desc = [[Associates a buffer with a socket. When a socket is buffered, data received on the socket is saved on the buffer. If the socket is not buffered, data received on the socket must be read as soon as possible or it might be lost. $NOTE$: buffered sockets are highly recommended for all applications.]],
      args = 
      {
        "$sock$ - the socket.",
        "$bufsize$ - the buffer size (0 to disable buffering)."
      }
    },    

    { sig = "err = #net.connect#( sock, ip, port, [timer_id, timeout])",
      desc = "Connect a socket to a remote system with optional timeout.",
      args = 
      {
        "$sock$ - a socket obtained from @#net.socket@net.socket@.",
        "$ip$ - the IP address obtained from @#net.packip@net.packip@.",
        "$port$ - the port to connecto to.",
        "$timer_id (optional)$ - the ID of the timer used for timed connect. If this is specified, $timeout$ must also be specified.",
        [[$timeout (optional)$ - timeout for the connect operation in microseconds. It can be 0 or the special value $net.INF_TIMEOUT$ (the function blocks until the connection is complete). The default if $net.INF_TIMEOUT$.]]
      },
      ret = "$err$ - the error code`, as defined @#error_codes@here@.`.`"
    },

    { sig = "socket, remoteip, err = #net.accept#( port, bufsize, [timer_id, timeout] )",
      desc = "Accept a connection from a remote system with an optional timeout.",
      args =
      {
        "$port$ - the port to wait for connections from the remote system.",
        "$bufsize$ - the size of the buffer that will be set on the socket returned from $accept$.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the accept function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $accept$ function returns if no connection was requested.]]
      },
      ret =
      {
        "$socket$ - the socket created after accepting the remote connection.",
        "$remoteip$ - the IP of the remote system.",
        "$err$ - the error code`, as defined @#error_codes@here@.`.`"
      }
    },

    { sig = "res, err = #net.send#( sock, str )",
      desc = "Send data to a TCP socket.",
      args = 
      {
        "$sock$ - the socket.",
        "$str$ - the data to send."
      },
      ret = 
      {
        "$res$ - the number of bytes actually sent or -1 for error.",
        "$err$ - the error code`, as defined @#error_codes@here@.`.`"
      }
    },

    { sig = "res, err = #net.recv#( sock, maxsize, [timer_id, timeout] )",
      desc = "Read data from a TCP socket.",
      args = 
      {
        "$sock$ - the socket.",
        "$maxsize$ - maximum size of the data to be received.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the recv function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $recv$ function returns if no data was received.]]
      },
      ret =
      {
        "$res$ - the number of bytes read.",
        "$err$ - the error code`, as defined @#error_codes@here@.`.`"
      }
    },

    { sig = "res = #net.set_split#( sock, [split_char] )",
      desc = [[Enables or disables 'split mode' on the given socket. If split mode is enabled and 'read' is called on the socket, the read will return as soon as the split character ($split_char$) is received. This can be used for example to read data from a socket line by line by setting $split_char$ to $\n$. $NOTE$: this function works only on buffered sockets.]],
      args = 
      {
        "$sock$ - the socket.",
        "$split_char$ - the split char, use $net.NO_SPLIT$ to disable splitting."
      }
    },

    { sig = "ip, netmask, dns, gw = #net.netcfg#()",
      desc = "Returns information about the network configuration.",
      ret =
      {
        "$ip$ - the IP address.",
        "$netmask$ - the network mask.",
        "$dns$ - the DNS server address.",
        "$gw$ - the default gateway address."
      }
    },

     
    { sig = "sentbytes, err = #net.sendto#( socket, data, remoteip, remoteport )",
      desc = "Sends data to an UDP socket.",
      args =
      {
        "$socket$ - the socket.",
        "$data$ - the data to be sent.",
        "$remoteip$ - the IP of the remote system (obtained from @#net.packip@net.packip@).",
        "$remoteport$ - the port of the remote system."
      },
      ret =
      {
        "$sentbytes$ - the number of bytes actually sent.",
        "$err$ - the error code`, as defined @#error_codes@here@.`.`"
      }
    },

    { sig = "data, err, remoteip, remoteport = #net.recvfrom#( sock, maxsize, [ timer_id, timeout ] )",
      desc = "Receives data from an UDP socket with optional timeout.",
      args =
      {
        "$sock$ - the socket.",
        "$maxsize$ - maximum size of the data to be received.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $recvfrom$ function returns if no data was received.]]
      },
      ret = 
      {
        "$data$ - the received data. This will be an empty string in case of error/timeout.",
        "$err$ - the error code`, as defined @#error_codes@here@.`.`",
        "$remoteip$ - the IP of the remote system.",
        "$remoteport$ - the port of the remote system."
      }
    },

    { sig = "res = #net.expect#( sock, s, [timer_id, timeout] )",
      desc = [[Reads from the given socket (either TCP or UDP) with optional timeout until string $s$ is received. The data before $s$ is ignored. $NOTE$: this function works only on buffered sockets.]],
      args =
      {
        "$sock$ - the socket.",
        "$s$ - the string to expect.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $expect$ function returns if string $s$ was not received.]]
      },
      ret = "$true$ if the function succeeded, $false$ otherwise." 
    },

     { sig = "data = #net.read_to#( sock, s, [timer_id, timeout] )",
      desc = [[Reads from the given socket (either TCP or UDP) with optional timeout until string $s$ is received. The data before $s$ is returned to the caller (without $s$). $NOTE$: this function works only on buffered sockets.]],
      args =
      {
        "$sock$ - the socket.",
        "$s$ - the string to look for.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $read_to$ function returns if string $s$ was not received.]]
      },
      ret = "The data received. This can be an empty string if $s$ was not found or an error/timeout occured." 
    },
   
     { sig = "data = #net.read_tags#( sock, tag1, tag2, [timer_id, timeout] )",
      desc = [[Reads from the given socket (either TCP or UDP) with optional timeout until string $tag1$ is received, then records the data received until string $tag2$ is received. The recorded data is returned to the caller (without $tag1$ or $tag2$). $NOTE$: this function works only on buffered sockets.]],
      args =
      {
        "$sock$ - the socket.",
        "$tag1$ - the first string to look for.",
        "$tag2$ - the second string to look for.",
        [[$timer_id (optional)$ - the timer ID of the timer used to timeout the function after a specified time. If this is specified, $timeout$ must also be specified.]],
        [[$timeout (optional)$ - the timeout, in microseconds, after which the $read_tagas$ function returns if $tag1$ or $tag2$ were not received.]]
      },
      ret = "The data received. This can be an empty string if $tag1$ or $tag2$ were not found or an error/timeout occured." 
    },

  },
}

data_pt = data_en
