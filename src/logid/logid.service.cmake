[Unit]
Description=Logitech Configuration Daemon

[Service]
Type=simple
ExecStart=${CMAKE_INSTALL_PREFIX}/bin/logid
User=root
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=multi-user.target
