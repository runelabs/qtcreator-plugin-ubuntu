#!/bin/bash
# Copyright 2013 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; version 2.1.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>

# prefix the logs with the username, so this does not clash on a multiuser machine
# Bug lp:1371807
echo $0 >> /tmp/${USER}_sdk.logs

set -e

SCRIPTPATH=`dirname $0`
SERIALNUMBER=$1
CONFIGDIR=~/.config/ubuntu-sdk
SSHIDENTITY=${CONFIGDIR}/ubuntudevice_id_rsa
USERNAME=phablet

function adb_wait {
	adb -s ${SERIALNUMBER} wait-for-device &> /dev/null
}

function has_ubuntu_chroot {
	local HAS_UBUNTU_CHROOT=`adb -s ${SERIALNUMBER} shell "which ubuntu_chroot"`
	if [[ ${HAS_UBUNTU_CHROOT} ]]; then
		echo 1
	else
		echo 0
	fi
}

function adbd_restart {
	adb kill-server &> /dev/null
	adb start-server &> /dev/null
}

function adb_devices {
	adb devices -l
}

function adb_forward_tcp {
set +e
	adb -s ${SERIALNUMBER} forward tcp:$1 tcp:$2
set -e
}

function adb_shell {
set +e
	if [[ `has_ubuntu_chroot` -eq "1" ]]; then
		adb -s ${SERIALNUMBER} shell chroot /data/ubuntu /usr/bin/env -i PATH=/bin:/usr/bin:/sbin:/usr/sbin:/tools/bin $@
	else
		adb -s ${SERIALNUMBER} shell $@
	fi
set -e
}

function phablet_shell {
	adb_shell "su ${USERNAME} -c \"$@\""
}

function phablet_tools_new_syntax {
        installed_version=$(dpkg-query --show --showformat '${Version}' phablet-tools)
        dpkg --compare-versions $installed_version ge 1.0
        echo $?
}

function ubuntu_stamp {
	adb_shell cat /etc/ubuntu-build
}

function device_hardware {
    adb_devices |grep -o device:.*|sed "s/device://"
}

function device_image_hardware {
    adb_shell getprop "ro.build.product"|tr -d '\r'
}

function does_device_hw_match_image_hw {
	if [[ `device_hardware` == `device_image_hardware` ]]; then
        	echo 1
	else
        	echo 0
	fi
}

function device_image_version {
	ubuntu_stamp 
}

function device_network_state_connected {
	local NM_STATE=`adb_shell nmcli nm | head -n+2 | awk '{print $2}'`
	if [[ $NM_STATE != *disconnected* ]]; then
	  echo 1
	else
	  echo 0
	fi
}

function wait_for_network_state_switch_on {
	local RESULT=`device_network_state_connected`
	while [[ "$RESULT" -eq "0" ]]; do
		sleep 1
		echo -n .
		RESULT=`device_network_state_connected`
	done
}

function clone_network_setup_from_host_to_device {
	pkexec phablet-network -s ${SERIALNUMBER}
}

#############################################################

if [[ -z ${SERIALNUMBER} ]]; then
  echo "Please pass a device serial number."
  echo
  adb_devices
  exit -1
fi

if [ ! -d ${CONFIGDIR} ]; then
  mkdir -p ${CONFIGDIR}
fi

adb_wait
