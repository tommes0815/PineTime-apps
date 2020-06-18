USEMODULE += gui
USEMODULE += controller
USEMODULE += widget
USEMODULE += fonts
#USEMODULE += bleman
USEMODULE += event_timeout
USEMODULE += hal
#USEMODULE += storage
USEMODULE += util

USEMODULE += shell
USEMODULE += shell_commands
#USEMODULE += ps
#USEMODULE += schedstatistics

# sensors
#USEMODULE += xpt2046
#USEMODULE += cst816s

# crypto
USEPKG+=wolfssl
USEMODULE+=wolfcrypt


# dp3t
USEMODULE+=dp3t

USEMODULE+=mtd
USEMODULE+=newlib

# network
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6
USEMODULE += gnrc_sock_udp
USEMODULE += sntp
#USEMODULE += periph_flashpage

# BLE
USEPKG += nimble
USEMODULE += nimble_scanner
USEMODULE += nimble_scanlist
USEMODULE += nimble_svc_gap
USEMODULE += nimble_svc_gatt

# update/secure boot
USEMODULE += libwolfboot
