#!/usr/bin/env sh

wget -P RIR ftp://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest
wget -P RIR ftp://ftp.ripe.net/ripe/stats/delegated-ripencc-latest
wget -P RIR ftp://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest
wget -P RIR ftp://ftp.apnic.net/pub/stats/apnic/delegated-apnic-latest
wget -P RIR ftp://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest
