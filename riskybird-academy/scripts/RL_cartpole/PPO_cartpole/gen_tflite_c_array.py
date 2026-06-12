def convert_to_c_array(filename, output_filename):
    with open(filename, 'rb') as f:
        content = f.read()

    array_name = filename.replace(".", "_").replace("-", "_")
    with open(output_filename, 'w') as f:
        f.write(f"alignas(8) const unsigned char {array_name}[] = {{\n")
        for i, byte in enumerate(content):
            if i % 12 == 0:
                f.write("\n    ")
            f.write(f"0x{byte:02x}, ")
        f.write("\n};\n")
        f.write(f"unsigned int {array_name}_len = {len(content)};\n")

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} [source_file] [output_file]")
        sys.exit(1)
    
    convert_to_c_array(sys.argv[1], sys.argv[2])
