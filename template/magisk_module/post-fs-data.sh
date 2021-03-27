#!/system/bin/sh
MODDIR=${0%/*}

# Remove this part if this module support Riru v24+ only

if [ -f "$MODDIR"/../riru-core/util_functions.sh ]; then
  # Riru v24
  # If this module is installed before Riru is updated to v24, we have to manually move the files to the new location
  [ -d "$MODDIR"/system ] && mv "$MODDIR"/system "$MODDIR"/riru
else
  # Riru pre-v24
  # In case user downgrade Riru to pre-v24
  [ -d "$MODDIR"/riru ] && mv "$MODDIR"/riru "$MODDIR"/system
fi
