# wifiExtender

Based on `ExampleRangeExtender-NAPT`

- Dele72.h
  - provides some 'secret' constants
- ESP8266WiFi.h
  - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#esp8266wifi-library
- WiFiClientSecure.h
  - is an extension to the standard client (of WiFiClient) using secure protocol
  - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#bearssl-client-secure-and-server-secure
  - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html
  - look at WiFi Multi
    - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#wifi-multi
    - to connect to a WiFi network with strongest WiFi signal (RSSI)
  - look at Memory requirements - DONE
    - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html#memory-requirements
    - TLS with MFLN
      - TLS benötigt eigentlich einen Buffer von 16KB zum Senden und Empfangen (scheinbar jeweils!)
      - ESP8266 hat aber nur einen heap von 40KB
      - Mit MFLN kann man kann den Buffer zum Senden auf etwas mehr als 512 bytes reduzieren
      - Der Buffer zum Empfangen kann jedoch nicht reduziert werden und bleibt bei 16KB
      - Nebenbei: NAPT benötigt scheinbar ca. 12552 bytes
      - [ReadTheDocs: MFLN (Saving RAM)](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html#mfln-or-maximum-fragment-length-negotiation-saving-ram)
      - [Example](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_MaxFragmentLength/BearSSL_MaxFragmentLength.ino)
    - **BUT**
      - my Webhoster doesn't support this :(
  - napt und webconnect funktioniert nun, wichtig sind folgende Werte:
    - `#define NAPT 64`, set max 64 (IP) entries in the NAPT table
    - `#define NAPT_PORT 32`, set max 32 portmap entries in the NAPT table
    - @TODO: genauer gucken was das bewirkt. Sind damit ganze Einträge gemeint oder z.B. bei der IP Adresse lediglich ein Tuple einer IP?
  - look at webconnect
    - aktuell funktioniert nur ein connect/POST, im nächsten loop ist der client weg - bei einem reconnect führt das zu einer exception
    - vielleicht hilft **TLS Session**
      - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html#sessions-resuming-connections-fast
      - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_Sessions/BearSSL_Sessions.ino
- lwip/napt.h
  - Network Address and Port Translation
  - https://github.com/esp8266/Arduino/blob/master/tools/sdk/lwip2/include/lwip/napt.h
  - **look at heap!**
    - in my case 12.880 (before: 36.104, after: 23.224)
    - after connecting to a Webserver by WiFiClientSecure is only 12.552 available (in my case)

## current Problems


### Scenario A: startNaptRouting() without connectWebserver()

wifi extending works like a charme
  
### Scenario B: if connectWebserver() and then startNaptRouting

connection to webserver throws no exception but napt failed and ap is available and connectable, but without internet connection

```
CONNECTING...
BSSL:_connectSSL: start connection
BSSL:Connected!

... returnVal: 1
Connected to server!
Server certificate verified
Heap before: 12504
ip_napt_init(1000,10): ret=-1 (OK=0)
Heap after napt init: 12504
NAPT initialization failed

My data string im POSTing looks like this: 
DEVICE=3&SSID=deva&RSSI=-50&HUMIDITY=deva&TEMPERATURE=-50&PRESSURE=bla&APPROXALTITUDE=bla&VOLTAGESOLAR=bla&VOLTAGEACCU=bla
And it is this many bytes: 
122
```

### Scenario C: if startNampRouting() and then connectWebserver(), this exception will be thrown:

```
CONNECTING...
BSSL:_connectSSL: start connection

User exception (panic/abort/assert)
--------------- CUT HERE FOR EXCEPTION DECODER ---------------

Abort called

>>>stack>>>

ctx: cont
sp: 3ffffd40 end: 3fffffc0 offset: 0000
3ffffd40:  7473203a 00000005 000001bd 40100a2d  
3ffffd50:  000000fe 00000000 00000000 00000000  
3ffffd60:  00000000 00000000 00000000 3fff0038  
3ffffd70:  00000000 00000000 00000020 3ffe9455  
3ffffd80:  3ffffee0 3ffefe70 00004145 40207786  
3ffffd90:  00000000 3ffefe70 00004145 40207798  
3ffffda0:  4025b2ac 3fff14e4 3ffefd78 402119e1  
3ffffdb0:  00000000 00000000 3ffefd60 4021198c  
3ffffdc0:  3ffefd60 3ffefe70 3ffefd60 40204a69  
3ffffdd0:  3fff0000 0000005a 0000009c 40100971  
3ffffde0:  00000000 00000000 3fff0440 4021b298  
3ffffdf0:  00009038 3fff0038 00000020 40100971  
3ffffe00:  00000014 3fff9cc4 000000ff 00000000  
3ffffe10:  00000006 3fff03fc 00000020 40100c94  
3ffffe20:  4010085c 3fff0038 000012ec 3fff9cc4  
3ffffe30:  3fff957c 3fff0440 00000000 4021b2e0  
3ffffe40:  3fff0440 00000000 00000000 401002d4  
3ffffe50:  00000000 4bc6a7f0 000027b6 4021b306  
3ffffe60:  3fff0440 00000000 00000000 4021beab  
3ffffe70:  3fff0184 00000008 4010026a 402172ee  
3ffffe80:  3fff0440 3fff0038 3fff0100 402130ed  
3ffffe90:  401051c1 003074e3 3fff9d48 00000000  
3ffffea0:  007a1200 34ce75ae 00307400 00000000  
3ffffeb0:  40105445 003074e3 3fff000c 00000000  
3ffffec0:  3ffeedd0 3fff000c 00000001 3ffefe4c  
3ffffed0:  00000016 3fff000c 00000001 40207218  
3ffffee0:  00000000 00000001 3ffefe4c 4020787e  
3ffffef0:  00000000 3ffefd60 3fff130c 40203060  
3fffff00:  3ffefe4c 00000000 00000d50 000001bb  
3fffff10:  3ffefd60 3ffe9455 3ffefe70 000001bb  
3fffff20:  3ffefd60 3ffe9455 00000001 40204d33  
3fffff30:  402099a8 2486ed6d 402099a8 2486ed6d  
3fffff40:  3ffefd5c 3ffefd60 3ffefe70 3ffeffac  
3fffff50:  3ffefd5c 3ffefd60 3ffefe70 402013c7  
3fffff60:  3fff1400 0032003f 80ffff40 3fff1400  
3fffff70:  0022002f 80fefe70 00000000 402012f0  
3fffff80:  3fffdad0 00000000 3ffefe34 4020149a  
3fffff90:  402099a8 00000000 feefeffe feefeffe  
3fffffa0:  feefeffe feefeffe 3ffeff6c 40207328  
3fffffb0:  feefeffe feefeffe 3ffe8ee0 40100f85  
<<<stack<<<

--------------- CUT HERE FOR EXCEPTION DECODER ---------------

 ets Jan  8 2013,rst cause:1, boot mode:(3,6)

load 0x4010f000, len 3584, room 16 
tail 0
chksum 0xb0
csum 0xb0
v2843a5ac
~ld
```

#### decoded

```
0x40100a2d: umm_malloc_core at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/umm_malloc/umm_malloc.cpp line 458
0x40207786: raise_exception at core_esp8266_postmortem.cpp line ?
0x40207798: __assert_func at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/core_esp8266_postmortem.cpp line 275
0x4025b2ac: chip_v6_unset_chanfreq at ?? line ?
0x402119e1: operator new(unsigned int) at /workdir/repo/gcc/libstdc++-v3/libsupc++/new_op.cc line 57
0x4021198c: operator new[](unsigned int) at /workdir/repo/gcc/libstdc++-v3/libsupc++/new_opv.cc line 33
0x40204a69: BearSSL::WiFiClientSecure::_connectSSL(char const*) at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/2.5.0-4-b40a506/xtensa-lx106-elf/include/c++/4.8.2/bits/shared_ptr_base.h line 463
:  (inlined by) ?? at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/2.5.0-4-b40a506/xtensa-lx106-elf/include/c++/4.8.2/bits/shared_ptr_base.h line 748
:  (inlined by) ?? at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/2.5.0-4-b40a506/xtensa-lx106-elf/include/c++/4.8.2/bits/shared_ptr.h line 130
:  (inlined by) BearSSL::WiFiClientSecure::_connectSSL(char const*) at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/libraries/ESP8266WiFi/src/WiFiClientSecureBearSSL.cpp line 1070
0x40100971: check_poison_neighbors$part$3 at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/umm_malloc/umm_local.c line 71
0x4021b298: ip4_output_if_opt_src at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/ipv4/ip4.c line 1764
0x40100971: check_poison_neighbors$part$3 at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/umm_malloc/umm_local.c line 71
0x40100c94: umm_malloc at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/umm_malloc/umm_malloc.cpp line 552
0x4010085c: get_poisoned at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/umm_malloc/umm_poison.c line 117
0x4021b2e0: ip4_output_if_opt at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/ipv4/ip4.c line 1577
0x401002d4: malloc at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/heap.cpp line 232
0x4021b306: ip4_output_if at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/ipv4/ip4.c line 1550
0x4021beab: ip_chksum_pseudo at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/inet_chksum.c line 395
0x4010026a: millis at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/core_esp8266_wiring.cpp line 188
0x402172ee: tcp_output at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/tcp_out.c line 1361
0x402130ed: sys_timeout_abs at /local/users/gauchard/arduino/arduino_esp8266/esp8266-lwip/tools/sdk/lwip2/builder/lwip2-src/src/core/timeouts.c line 189
0x401051c1: wdt_feed at ?? line ?
0x40105445: ets_timer_arm_new at ?? line ?
0x40207218: esp_yield at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/core_esp8266_main.cpp line 119
0x4020787e: __delay at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/core_esp8266_wiring.cpp line 54
0x40203060: WiFiClient::connect(IPAddress, unsigned short) at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/libraries/ESP8266WiFi/src/include/ClientContext.h line 144
:  (inlined by) WiFiClient::connect(IPAddress, unsigned short) at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/libraries/ESP8266WiFi/src/WiFiClient.cpp line 170
0x40204d33: BearSSL::WiFiClientSecure::connect(char const*, unsigned short) at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/libraries/ESP8266WiFi/src/WiFiClientSecureBearSSL.cpp line 231
0x402099a8: precache at ?? line ?
0x402099a8: precache at ?? line ?
0x402013c7: connectWebserver() at /home/USERNAME/dev/arduino/sketches/esp8266/wifiExtender/wifiExtender.ino line 235
0x402012f0: startNaptRouting() at /home/USERNAME/dev/arduino/sketches/esp8266/wifiExtender/wifiExtender.ino line 192
0x4020149a: setup at /home/USERNAME/dev/arduino/sketches/esp8266/wifiExtender/wifiExtender.ino line 88
0x402099a8: precache at ?? line ?
0x40207328: loop_wrapper() at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/core_esp8266_main.cpp line 194
0x40100f85: cont_wrapper at /home/USERNAME/snap/arduino/56/.arduino15/packages/esp8266/hardware/esp8266/2.7.4/cores/esp8266/cont.S line 81
```

