#!/bin/sh

VERSION=2.5

# Determine how to use echo without a carriage return.
# This is pretty much stolen from autoconf.

if (echo "testing\c"; echo 1,2,3) | grep c >/dev/null
then
	if (echo -n testing; echo 1,2,3) | sed s/-n/xn/ | grep xn >/dev/null
	then
		ac_n= ac_c='
' ac_t=' '
	else
		ac_n=-n ac_c= ac_t=
	fi
else
	ac_n= ac_c='\c' ac_t=
fi         

#
# Create sub-directories if they don't exist.
#
make_subdirs () {
	path=""
	for dir in `echo $1 | sed -e 's,/, ,g'`; do
		path="$path/$dir"
		if [ ! -d $path ]; then
			mkdir $path
			if [ $? -ne 0 ]; then
				echo "Could not create $path." 1>&2
				exit 1
			fi
		fi
	done
}

#
# Prompt for a yes/no answer.
#
prompt_yn () {
	
	prompt=$1
	default=$2

	while true
	do
		echo $ac_n \
			"$prompt [$default] $ac_c" 1>&2

		read yn_flag

		if [ "$yn_flag" = "" ]; then
			yn_flag=$default
		fi

		yn_flag=`echo $yn_flag | tr '[a-z]' '[A-Z]'`

		if [ $yn_flag = "Y" -o $yn_flag = "YES" ]
		then
			return 0
		fi

		if [ $yn_flag = "N" -o $yn_flag = "NO" ]
		then
			return 1
		fi

		echo "Response must be y/n." 1>&2
	done

}


#
# Prompt path. Simple function to prompt user for the name
# of a path that is to be used for installation.
#
prompt_path () {

	var_name=$1
	prompt=$2
	default_path=$3

	path_done=0
	while [ $path_done = 0 ]
	do
		path_done=1

		echo $ac_n "$prompt [$default_path] $ac_c"
		read input_path

		if [ "$input_path" = "" ]; then
			input_path=$default_path
		fi

		if [ ! -d $input_path ]
		then
			echo $ac_n "   ==> Directory $input_path does not exist, create it? [y] $ac_c"
			read yn

			if [ "$yn" = "" -o "$yn" = "y" -o "$yn" = "Y" -o "$yn" = "yes" ]; then
				make_subdirs $input_path
			else
				path_done=0
			fi
		fi
	done

	eval "$var_name=$input_path"
}

# Startup installation banner...

echo "+============================================================+"
echo "|                                                            |"
echo "|             SQSH-$VERSION BINARY INSTALLATION SCRIPT            |"
echo "|                                                            |"
echo "+============================================================+"
echo
echo "The following shell script will prompt you for several"
echo "pieces of information required in order to properly install"
echo "sqsh.  At any time during the installation process you may"
echo "abort by hitting ^C at a prompt"
echo
echo $ac_n "Hit ENTER to continue: $ac_c"
read x

root_path=/usr/local
sqshrc_path=
man_path=
ld_path=

config_done=0
while [ $config_done = 0 ]
do
	config_done=1

	echo "=================[ ROOT DIRECTORY ]======================"
	echo
	echo "Please input root directory in which the sqsh binary is to be"
	echo "installed. Typical locations are /usr, /usr/local, or /opt/sqsh."
	echo

	prompt_path root_path "==> Root installation directory" $root_path

	if [ ! -d $root_path/bin ]; then
		make_subdirs $root_path/bin
	fi

	echo
	echo "=================[ GLOBAL CONFIGURATION ]======================"
	echo
	echo "With this release, sqsh allows a global sqshrc file to be"
	echo "installed to set default configuration values for all users."
	echo "By default this file will be processed prior to each user's"
	echo "personal \$HOME/.sqshrc file.  The global sqshrc file will"
	echo "be placed in the following directory."
	echo

	if [ "$sqshrc_path" = "" ]; then
		sqshrc_path=$root_path/etc
	fi

	prompt_path sqshrc_path \
		"==> Global sqshrc installation directory" $sqshrc_path

	echo
	echo "======================[ MANUAL PAGE ]=========================="
	echo
	echo "The following provides the location for the sqsh manual page."
	echo "This page will be installed under the name sqsh.1 in the"
	echo "supplied location.  Note that any user wanting to read this"
	echo "manual should have this directory in their MANPATH environment"
	echo "variable"
	echo

	if [ "$man_path" = "" ]; then
		man_path=$root_path/man/man1
	fi
	
	prompt_path man_path \
		"==> Manual installation directory" $man_path

	echo
	echo "======================[ WRAPPER SET-UP ]=========================="
	echo
	echo "With the binary only installation, a shell-script wrapper for"
	echo "sqsh is installed.  This wrapper does things such as set the"
	echo "\$SQSHRC environment variable with the global sqshrc path"
	echo "supplied previously ($sqshrc_path). For the sake of less"
	echo "knowledgable individuals (or the sake the sanity of your"
	echo "system administrator), this script may also set your"
	echo "LD_LIBRARY_PATH or SHLIB_PATH environment variables."
	echo "If you don't know what these variables do, answer 'y', below."
	echo
	
	if prompt_yn "==> Set LD_LIBRARY_PATH or SHLIB_PATH in wrapper?" y
	then
		do_ld_path="yes"
	else
		do_ld_path="no"
	fi

	if [ "$do_ld_path" = "yes" ]
	then
		if [ "$ld_path" = "" ]; then
			#
			# See if the X11 path used to compile the sqsh binary on
			# the source machine matches that of the machine we are
			# install on.
			#
			x11_path=""
			if [ "yes" = "yes" ]
			then
				x11_path=""
				for dir in \
					`echo # -L/usr/X11R6/lib | sed -e 's,-L,,g'` \
					/usr/lib/X11 \
					/usr/openwin/lib \
					/usr/local/lib \
					/usr/local/lib/X11 \
					/usr/X11R6/lib \
					/usr/X11R5/lib
				do
					if [ -f $dir/libXt.a -o \
						  -f $dir/libXt.so -o \
						  -f $dir/libXt.sl ]
					then
						x11_path="$dir"
						break;
					fi
				done
			fi

			motif_path=""
			if [ "no" = "yes" ]
			then
				motif_path=""
				for dir in \
					`echo  | sed -e 's,-L,,g'` \
					/usr/lib/X11 \
					/usr/openwin/lib \
					/usr/local/lib \
					/usr/local/lib/X11 \
					/usr/X11R6/lib \
					/usr/X11R5/lib
				do
					if [ -f $dir/libXm.a -o \
						  -f $dir/libXm.so -o \
						  -f $dir/libXm.sl ]
					then
						motif_path="$dir"
						break;
					fi
				done
			fi

			readline_path=""
			if [ "yes" = "yes" ]
			then
				for dir in `echo  | sed -e 's,^-L,,g'` \
					/usr/local/lib /usr/lib
				do
					if [ -f $dir/libreadline.a -o \
						-f $dir/libreadline.so -o \
						-f $dir/libreadline.sl ]
					then
						readline_path="$dir"
					fi
				done
			fi

			#
			# Attempt to track down where open client is installed.
			#
			for dir in $SYBASE $SYBASE/OCS $SYBASE/OCS-[0-9]*
			do
				if [ -f $dir/lib/libct.a -o \
					  -f $dir/lib/libct.so -o \
					  -f $dir/lib/libct.sl ]; then
					sybase_path="$dir/lib"
					break
				fi
			done

			ld_path="$sybase_path"

			if [ "$x11_path" != "" ]; then
				ld_path="$ld_path:$x11_path"
			fi

			if [ "$motif_path" != "" -a "$motif_path" != "$x11_path" ]; then
				ld_path="$ld_path:$motif_path"
			fi

			if [ "$readline_path" != "" ]; then
				ld_path="$ld_path:$readline_path"
			fi

			#
			# I hate useless entries.
			#
			ld_path=`echo $ld_path | sed -e 's,::,:,g'`
		fi

		echo
		echo "The following LD_LIBRARY_PATH has been selected for you"
		echo "by examining your current operating environment for the"
		echo "libraries required to use the features that are currently"
		echo "compiled into your sqsh binary. If you wish to leave it "
		echo "unchanged, simply hit enter, otherwise, enter an updated "
		echo "path you wish to use instead."

		ld_done="no"
		while [ "$ld_done" = "no" ]; do
			ld_done="yes"

			echo
			echo "LD_LIBRARY_PATH = $ld_path"
			echo $ac_n "==> New path: $ac_c " 1>&2
			read path

			if [ "$path" = "" ]; then
				path=$ld_path
			else
				ld_path=$path
			fi

			for dir in `echo $ld_path | sed -e 's,:, ,g'`
			do
				if [ ! -d $dir ]; then
					echo "   *** Directory $dir does not exist." 1>&2
					ld_done="no"
				fi
			done
		done
	else
		ld_path="<not set>"
	fi

	echo
	echo "======================[ VERIFICATION ]=========================="
	echo "Root directory:             $root_path"
	echo "Global sqshrc installation: $sqshrc_path/sqshrc"
	echo "Manual installation:        $man_path/sqsh.1"
	echo "Library Path:               $ld_path"
	echo

	if prompt_yn "==> Are these settings correct?" y
	then
		echo "Here we go!"
	else
		config_done=0
	fi
done

# First, install the binary

echo "Installing binary..."
cp sqsh $root_path/bin/sqsh
if [ $? -ne 0 ]; then
	echo "Unable to copy binary into $root_path/bin. Aborting" 1>&2
	exit 1
fi
chmod gou+rx $root_path/bin/sqsh

# Next, install the global sqshrc

echo "Installing global sqshrc..."
cp doc/global.sqshrc $sqshrc_path/sqshrc
if [ $? -ne 0 ]; then
	echo "Unable to copy global sqshrc into $sqshrc_path. Aborting" 1>&2
	exit 1
fi
chmod gou+r $sqshrc_path/sqshrc

# Install the man page script

echo "Installing manual..."
cp doc/sqsh.1 $man_path/sqsh.1
if [ $? -ne 0 ]; then
	echo "Unable to copy manual into $man_path. Aborting" 1>&2
	exit 1
fi
chmod gou+r $man_path/sqsh.1

echo "Installing wrapper script..."
mv $root_path/bin/sqsh $root_path/bin/sqsh.bin && \
sed -e "s,%SYBASE%,$SYBASE,g" \
	 -e "s,%LD_LIBRARY_PATH%,$ld_path,g" \
	 -e "s,%BIN_DIR%,$root_path/bin,g" \
	 -e "s,%SQSHRC%,$sqshrc_path/sqshrc:\${HOME}/.sqshrc,g" \
	 scripts/wrapper.sh.in > $root_path/bin/sqsh
chmod gou+rx $root_path/bin/sqsh

if [ $? -ne 0 ]; then
	echo "Error creating wrapper in $root_path/bin. Aborting" 1>&2
	exit 1
fi

echo "Done."
