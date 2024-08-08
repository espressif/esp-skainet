import re

# 输入字符串
input_string = "/home/sunxiangyu/workspace/esp-skainet/examples/wake_word_detection/main/main.c:85:5: error: expected ',' or ';' before 'if'\n   85 |     if (models!=NULL) {\n      |     ^~\n/home/sunxiangyu/workspace/esp-skainet/examples/wake_word_detection/main/main.c:97:7: error: 'else' without a previous 'if'\n   97 |     } else {\n      |       ^~~~"

# 正则表达式模式
pattern = r"^(.*?):(\d+):(\d+): error: (.*?)\n"
#pattern = r"^(.*?):(\d+):(\d+): error: (.*?)\n\s*(\d+) \| (.*?)\n\s*\| (.*?)\n"

# 查找所有匹配项
matches = re.findall(pattern, input_string, re.MULTILINE)

# 打印结果
match = matches[0]
file_name = match[0]
line_number = match[1]
column_number = match[2]
error_message = match[3]
#error_line_number = match[4]
#error_line_content = match[5]
#error_column_indicator = match[6]

print("文件名:", file_name)
print("报错位置: 行", line_number, "列", column_number)
print("错误信息:", error_message)
#print("错误行号:", error_line_number)
#print("错误行内容:", error_line_content)
#print("错误列指示器:", error_column_indicator)
print("---")
