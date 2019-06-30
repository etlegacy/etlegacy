#!/bin/sh

# Imagemagick's world flags generator

# Country codes : https://github.com/maxmind/geoip-api-c/blob/master/libGeoIP/GeoIP.c
# Flag icon set : https://www.gosquared.com/resources/flag-icons/
#                 https://github.com/gosquared/flags
#
# Put _bots.png in flags-iso/shiny/32 directory.
# Put the script in flags-iso/shiny/32 and execute.
# Copy generated world_flags.tga file to /etmain/gfx/flags/.

items=0
current_row=""
total_rows=0

# changed "--" to "00" as cp doesn't like it
geoip_country_codes=("00" "AP" "EU" "AD" "AE" "AF"
	"AG" "AI" "AL" "AM" "CW"
	"AO" "AQ" "AR" "AS" "AT" "AU"
	"AW" "AZ" "BA" "BB"
	"BD" "BE" "BF" "BG" "BH" "BI"
	"BJ" "BM" "BN" "BO"
	"BR" "BS" "BT" "BV" "BW" "BY"
	"BZ" "CA" "CC" "CD"
	"CF" "CG" "CH" "CI" "CK" "CL"
	"CM" "CN" "CO" "CR"
	"CU" "CV" "CX" "CY" "CZ" "DE"
	"DJ" "DK" "DM" "DO"
	"DZ" "EC" "EE" "EG" "EH" "ER"
	"ES" "ET" "FI" "FJ"
	"FK" "FM" "FO" "FR" "SX" "GA"
	"GB" "GD" "GE" "GF"
	"GH" "GI" "GL" "GM" "GN" "GP"
	"GQ" "GR" "GS" "GT"
	"GU" "GW" "GY" "HK" "HM" "HN"
	"HR" "HT" "HU" "ID"
	"IE" "IL" "IN" "IO" "IQ" "IR"
	"IS" "IT" "JM" "JO"
	"JP" "KE" "KG" "KH" "KI" "KM"
	"KN" "KP" "KR" "KW"
	"KY" "KZ" "LA" "LB" "LC" "LI"
	"LK" "LR" "LS" "LT"
	"LU" "LV" "LY" "MA" "MC" "MD"
	"MG" "MH" "MK" "ML"
	"MM" "MN" "MO" "MP" "MQ" "MR"
	"MS" "MT" "MU" "MV"
	"MW" "MX" "MY" "MZ" "NA" "NC"
	"NE" "NF" "NG" "NI"
	"NL" "NO" "NP" "NR" "NU" "NZ"
	"OM" "PA" "PE" "PF"
	"PG" "PH" "PK" "PL" "PM" "PN"
	"PR" "PS" "PT" "PW"
	"PY" "QA" "RE" "RO" "RU" "RW"
	"SA" "SB" "SC" "SD"
	"SE" "SG" "SH" "SI" "SJ" "SK"
	"SL" "SM" "SN" "SO"
	"SR" "ST" "SV" "SY" "SZ" "TC"
	"TD" "TF" "TG" "TH"
	"TJ" "TK" "TM" "TN" "TO" "TL"
	"TR" "TT" "TV" "TW"
	"TZ" "UA" "UG" "UM" "US" "UY"
	"UZ" "VA" "VC" "VE"
	"VG" "VI" "VN" "VU" "WF" "WS"
	"YE" "YT" "RS" "ZA"
	"ZM" "ME" "ZW" "A1" "A2" "01"
	"AX" "GG" "IM" "JE"
	"BL" "MF" "BQ" "SS" "01")

# handle special territories
cp -fpv _bots.png 00.png # was '--'
cp -f _unknown.png AP.png
cp -f NO.png BV.png
cp -f NL.png SX.png
cp -f FR.png GF.png
cp -f FR.png GP.png
cp -f AU.png HM.png
cp -f GB.png IO.png
cp -f FR.png PM.png
cp -f FR.png RE.png
cp -f NO.png SJ.png
cp -f US.png UM.png
cp -f NL.png BQ.png
cp -f _united-nations.png A1.png
cp -f _united-nations.png A2.png
cp -fpv _united-nations.png 01.png # localhost

for i in ${geoip_country_codes[@]};
do
		items=$((items+1))
		current_row+="${i}.png "

		if [ $(( ${items} % 16 )) -eq 0 ] || [ ${items} -eq 256 ]; then
		printf "Stitching items on row ${total_rows}: ${current_row}\n"
		total_rows=$((total_rows+1))
		convert ${current_row} +append row${total_rows}.png
		current_row=""
		fi
done

rows=""

for ((i=1;i<=${total_rows};i++));
do
		rows+="row${i}.png "
done

printf "Stitching rows: ${rows}\n"
convert ${rows} -append flags.png
convert flags.png world_flags.tga
