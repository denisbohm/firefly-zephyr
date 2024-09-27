#!/bin/zsh

# The following or similar should setup the PATH to have the right protobuf binaries:
# toolchain=/opt/nordic/ncs/toolchains/580e4ef81c
# export PATH=$toolchain/bin:$toolchain/opt/nanopb/generator-bin:$PATH

setopt extended_glob

proto_path=/opt/nordic/ncs/v2.7.0/modules/lib/nanopb/generator/proto

c_out=`pwd`/../subsys/rpc/protobuf
python_out=`pwd`/python
swift_out=`pwd`/swift

cd protobuf

echo "compiling options.proto"
protoc --proto_path=. --python_out=. options.proto

echo "generating C sources"
proto_sources=(*.proto~options.proto)
protoc --proto_path=. --proto_path=${proto_path} --nanopb_out=${c_out} options.proto
protoc --proto_path=. --proto_path=${proto_path} --nanopb_out=${c_out} ${proto_sources}
protoc --proto_path=. --proto_path=${proto_path} --plugin=protoc-gen-custom-plugin=`pwd`/plugin_c.py --custom-plugin_out=${c_out} ${proto_sources}

echo "generating python sources"
protoc --proto_path=. --proto_path=${proto_path} options.proto ${proto_path}/nanopb.proto --python_out=${python_out}
protoc --proto_path=. --proto_path=${proto_path} --python_out=${python_out} ${proto_sources}
protoc --proto_path=. --proto_path=${proto_path} --plugin=protoc-gen-custom-plugin=`pwd`/plugin_python.py --custom-plugin_out=${python_out} ${proto_sources}

echo "generating swift sources"
protoc --proto_path=. --proto_path=${proto_path} --swift_out=${swift_out} ${proto_sources}
protoc --proto_path=. --proto_path=${proto_path} --plugin=protoc-gen-custom-plugin=`pwd`/plugin_swift.py --custom-plugin_out=${swift_out} ${proto_sources}
