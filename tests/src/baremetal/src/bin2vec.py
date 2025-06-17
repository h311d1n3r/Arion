import lief
import sys
import textwrap

def bytes_to_c_array(byte_list, indent=4, line_size=12):
    c_array = "unsigned char shellcode[] = {\n"
    pad = " " * indent

    for i in range(0, len(byte_list), line_size):
        chunk = byte_list[i:i+line_size]
        hex_bytes = ",".join(f"0x{b:02x}" for b in chunk)
        c_array += f"{pad}{hex_bytes},\n"

    c_array = c_array.rstrip(",\n") + "\n};\n"
    return c_array

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 bin2vec.py <binary_file>")
        sys.exit(1)

    binary_path = sys.argv[1]

    try:
        binary = lief.parse(binary_path)
    except Exception as e:
        print(f"Failed to parse binary: {e}")
        sys.exit(1)

    text_section = binary.get_section(".text")
    if not text_section:
        print("No .text section found.")
        sys.exit(1)

    result = bytes_to_c_array(text_section.content)
    print(result)

if __name__ == "__main__":
    main()
