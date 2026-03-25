read -p "1. -> m5stack-core-esp32
2. -> m5stack-core-esp32-16M

"

if [[ $REPLY =~ 1 ]]; then
  pio run -e m5stack-core-esp32 --target upload
fi

if [[ $REPLY =~ 2 ]]; then
  pio run -e m5stack-core-esp32-16M --target upload
fi
