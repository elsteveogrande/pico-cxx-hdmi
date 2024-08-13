# pico hdmi

## quick start


### install necessary tools

```
brew install cmake ninja
brew install --cask gcc-arm-embedded
export PATH="/opt/homebrew/bin:${PATH}"
```

### get source

```
git clone git@github.com:elsteveogrande/pico-cxx-hdmi.git
cd pico-cxx-hdmi

# init pico-sdk submodule, and other submodules within that
# (skip if you already have a working pico-sdk as mentioned above)
git submodule update --init --recursive
```

### build

```
# go to this repo root
cd ~/code/pico-cxx-hdmi
./build.sh
```
