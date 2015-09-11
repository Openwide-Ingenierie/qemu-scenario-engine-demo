#! /bin/bash

# Written by Victor Clément <victor.clement@openwide.fr>
#
#
# This script runs the scenario engine demonstration automatically.
#
# Use the "reset" option to delete qemu git repo and trace files.


QEMU_REPO="git://git.qemu-project.org/qemu.git"

demo_step() {
	echo ""
	echo -e "\033[1m*** $1 ***\033[0m"
}

has() {
	type "$1" >/dev/null 2>&1
}

demo_reset() {
	demo_step "Reset mode"
	echo "This will delete the 'qemu' and 'traces' directories and the 'trace.vcd' file"
	printf "Reset the demo now ? [y/N] "
	read ANS
	if [[ "$ANS" == "y" || "$ANS" == "Y" ]] ; then
		echo "Resetting directory"
		rm -rf qemu traces trace.vcd 2>&1 >> /dev/null
	else
		echo "Aborted"
	fi
	demo_step "Done"
}

# Start demo script
cd "$(dirname "$0")"

if [ $# -ge 1 ] && [ $1 = "reset" ] ; then
	demo_reset
	exit $?
fi

# Clone Qemu
if [ ! -d "qemu" ]; then
	demo_step "Cloning Qemu into \"qemu\""
	git clone "$QEMU_REPO" qemu
	cd qemu
	git checkout v2.4.0
else
	cd qemu
fi

# Patch Qemu
if [ ! -d "scenario" ]
then
	demo_step "Patching Qemu with the scenario engine"
	git apply ../scenario_engine_patch/*
fi

# Configure if needed
if [ ! -f "bin/arm-scenario/config.status" ]
then
	demo_step "Configuring Qemu"
	# Check for configure options
	CONFIG_OPTS=""
	PYTHON2_PATH=$(which python2)
	if [ ! -z $PYTHON2_PATH ]
	then
		echo "Add python config opt to: $PYTHON2_PATH"
		CONFIG_OPTS+="--python=$PYTHON2_PATH"
	fi

	# Configure
	mkdir -p bin/arm-scenario && cd bin/arm-scenario
	../../configure \
		--enable-debug \
		--target-list=arm-softmmu \
		--audio-drv-list= \
		--enable-trace-backend=simple \
		--enable-scenario-engine \
		$CONFIG_OPTS

	CONFIGURE_RET=$?
	if [ $CONFIGURE_RET -ne 0 ] ; then
		echo "Error configuring Qemu, abort demo"
		exit $CONFIGURE_RET
	fi
else
	cd bin/arm-scenario
fi

# Calculate make -j option
demo_step "Building Qemu"
MAKE_OPTS=""
if has nproc
then
	MAKE_OPTS+="-j$(($(nproc)+1))"
fi

# Build
make $MAKE_OPTS
MAKE_RET=$?
if [ $MAKE_RET -ne 0 ] ; then
	echo "Error building Qemu, abort demo"
	exit $MAKE_RET
fi

#Run
demo_step "Running Qemu"
cd ../../..
./qemu/bin/arm-scenario/arm-softmmu/qemu-system-arm \
	-M versatilepb \
	-kernel guest/irq_timer_gpio/program.bin \
	-trace events=events \
	-icount shift=0,nosleep \
	-scenario file=simu.sef \
	-display none \
	& \

QEMU_PID=$!
sleep 3
kill -TERM $QEMU_PID
sleep 1

# Generate VCD
demo_step "Generating VCD"
if [ -f "trace-$QEMU_PID" ]
then
	mkdir -p "traces"
	mv "trace-$QEMU_PID" traces
	./qemu/scripts/gpiotrace_ns.py qemu/trace-events traces/trace-$QEMU_PID
	if [ $? -eq 0 ] ; then
		echo "trace.vcd generated"
	else
		echo "Error generating trace.vcd"
	fi
else
	echo "Error: raw trace file not found"
	exit 1
fi

# Visualize
if has gtkwave
then
	printf "Open GTK Wave now ? [Y/n] "
	read ANS
	if [[ "$ANS" == "" || "$ANS" == "y" || "$ANS" == "Y" ]] ; then
		gtkwave trace.vcd
	fi
fi

demo_step "Done"

