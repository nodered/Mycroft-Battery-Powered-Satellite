#!/bin/bash

#    Script to map two pulseaudio hardware input sources as mono inputs
#    to Mic1 and Mic2 channel of a new loopback-sink respectively. This
#    sink can be used e.g. to use VoIP or record two microphones seperately.
#    CopyMic2 (C) 2013, Henning Hollermann, laclaro@mail.com
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

do_activate() {
    while [ "x" = "x$Mic1" ]; do
        echo "Choose first microphone by ID"
        pactl list short sources
        read ID
        Mic1=$(pactl list short sources|awk '/^'$ID'/{print $2}')
    done
    while [ "x" = "x$Mic2" ]; do
        echo "Choose second microphone by ID"
        pactl list short sources | grep -v $Mic1
        read ID
        Mic2=$(pactl list short sources | grep -v $Mic1|awk '/^'$ID'/{print $2}')
    done
    # Create the name of the Combined sink
    NAME="Combined_Mics:_Mic1:_"$(echo $Mic1|awk -F'.' '$0=$2')"_Mic2:_"$(echo $Mic2|awk -F'.' '$0=$2')

    echo "[LOAD] null sink as \"$NAME\" to connect the two mics to"
    pactl load-module module-null-sink \
            sink_name=combined \
            sink_properties="device.description=$NAME"

    echo "[LOAD] map source 1 ($Mic1) to Mic1 channel of \"$NAME\""
    pactl load-module module-remap-source master=$Mic1
    pactl load-module module-loopback sink=combined source=${Mic1}

    echo "[LOAD] map source 2 ($Mic2) to Mic2 channel of \"$NAME\""
    pactl load-module module-remap-source master=$Mic2
    pactl load-module module-loopback sink=combined source=${Mic2}
    echo "[DONE] Now adjust the Mic1 and Mic2 channel volume of the new sink to be equally loud"


}

do_deactivate() {
    echo "[UNLOAD] pulseaudio modules..."
    echo "[UNLOAD] module-loopback"
    pactl unload-module module-loopback
    echo "[UNLOAD] module-remap-source"
    pactl unload-module module-remap-source
    echo "[UNLOAD] module-null-sink"
    pactl unload-module module-null-sink
}

init() {
    for exe in /usr/bin/pulseaudio /usr/bin/pactl; do
        if [ ! -x "$exe" ]; then
            echo "[ERROR] required file $exe not found or not executable"
            exit 1
        fi
    done
    [ ! -x /usr/bin/pavucontrol ] && echo "[NOTICE] pavucontrol might be very useful."
}

# MAIN
init;
case $1 in
activate|enable|start)
    do_activate;;
deactivate|disable|stop)
    do_deactivate;;
*)
    echo "Usage: $0 [enable|disable]";;
esac;
