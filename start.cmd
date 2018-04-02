@echo off

set MAP_DRV_NO=Z

if exist %MAP_DRV_NO%: subst %MAP_DRV_NO%: /d 
subst %MAP_DRV_NO%: .

cd /d %MAP_DRV_NO%:

cmd /k

