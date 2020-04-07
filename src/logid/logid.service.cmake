[Unit]
Description=Logitech Configuration Daemon

[Service]
Type=simple
ExecStart=${CMAKE_INSTALL_PREFIX}/bin/logid
User=root
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
StartLimitIntervalSec=0

[Install]
WantedBy=multi-user.target
