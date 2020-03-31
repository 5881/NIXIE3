# Проект nixie часов на esp12e (esp8266)
Проект состоит из трёх частей
* nixie3 прошивка для часов с синхронизаций с серверами точного времени
* client прошивка реализующая внешнее nixie табло для компьютера 
работающее через сеть
* server серверная часть табло работает на linux

Код nixie3 и client написан на C с использованием проэкта 
[esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
для archlinux в AUR есть PKGBUILD https://aur.archlinux.org/packages/esp-open-sdk-git/

server собирается с помощью gcc 9.2

прошивка esp12e осуществляется с помощью esptool и USB-UART преобразователя
на ft232rl возможно использование другого преобразователя, отредактировав 
настройки скорости соответствующем Makefile
