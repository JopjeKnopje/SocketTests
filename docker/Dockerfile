FROM nginx:latest

RUN apt update && apt install vim php-fpm -y 

COPY nginx.conf /etc/nginx/conf.d

CMD ["/bin/bash", "-c", "php-fpm8.2 && chmod 755 /usr/share/nginx/html/* && nginx -g 'daemon off;'"]
