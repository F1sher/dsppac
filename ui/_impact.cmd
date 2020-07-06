setPreference -pref AutoSignature:FALSE
setPreference -pref KeepSVF:FALSE
setPreference -pref ConcurrentMode:FALSE
setPreference -pref UseHighz:FALSE
setPreference -pref svfUseTime:FALSE
setMode -bs
setMode -bs
setCable -port auto
Identify 
identifyMPM 
setMode -bs
addDevice -p 1 -file "/home/vukap/VUKAP/dsppac/firmwares/ezusb1_wakeup_newAD.bit"
deleteDevice -position 2
Program -p 1 -s -defaultVersion 0 
