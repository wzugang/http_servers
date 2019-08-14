#user  nobody;
worker_processes  1;

#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;

events {
   # worker_connections  1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;

    add_header Access-Control-Allow-Origin *;
    add_header Access-Control-Allow-Headers X-Requested-With;
    add_header Access-Control-Allow-Methods GET,POST,OPTIONS;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log  logs/access.log  main;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    keepalive_timeout  65;

    gzip  on;

	server {
        client_max_body_size 4G;
        listen  80;  ## listen for ipv4; this line is default and implied
        server_name _; ## localhost
		charset      utf-8,gbk;
		
        root C:\Users\vip\Desktop\httptools\curl;
		location / {
         autoindex on;
         autoindex_exact_size on;
         autoindex_localtime on;
        }
	}
}




