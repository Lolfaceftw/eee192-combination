import binascii
import time

TIME_DELAY = 1.01

filename = "putty.log"
def byte_to_string(line: bytes) -> str:
    return line.replace(starting_bits, b'\n' + starting_bits).decode("utf-8")

# ! The commented portions assume data is represented in integers.
# def string_to_hex(string: str, data_num: int) -> str:
#     first_part = string[0:2]
#     second_part = string[2:4]
#     #print(f"Data {data_num} Hex: {string} | {first_part} | {second_part}")
#     return f"{int(first_part, 16)}.{int(second_part, 16)}"


while True:
    with open(filename, 'rb') as f:
        content = f.read()
    hex_content = binascii.hexlify(content)
    starting_bits = b'424d'
    
    last_line_b = hex_content.split(starting_bits)[::-1][0]
    last_line_str = byte_to_string(last_line_b)
    # !The last line does not include 0x424d
    # We choose the particles under atmospheric conditions per the datasheet.
    data_1 = last_line_str[16:20]
    data_2 = last_line_str[20:24]
    data_3 = last_line_str[24:28]
    with open("convert.log", "a") as f:
        # print(f"Hex: {last_line_str}")
        # print(f"Hex: {last_line_str}", file=f)
        try:
            # print(f"PM1.0: {string_to_hex(data_1, 1)} | PM2.5: {string_to_hex(data_2, 2)} | PM10: {string_to_hex(data_3, 3)} || Unit: ug/m3")
            # print(f"PM1.0: {string_to_hex(data_1, 1)} | PM2.5: {string_to_hex(data_2, 2)} | PM10: {string_to_hex(data_3, 3)} || Unit: ug/m3", file=f)
            # print("-"*10, "No decimal", "-"*10)
            print(f"PM1.0: {int(data_1, 16)} | PM 2.5: {int(data_2, 16)} | PM 10: {int(data_3, 16)} || Unit: ug/m3")
            print(f"PM1.0: {int(data_1, 16)} | PM 2.5: {int(data_2, 16)} | PM 10: {int(data_3, 16)} || Unit: ug/m3", file=f)
        except:
            print("Error! Bits are short... looping again...")
            print("Error! Bits are short... looping again...", file=f)
            time.sleep(TIME_DELAY)
            continue
    time.sleep(TIME_DELAY)