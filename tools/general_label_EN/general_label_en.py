# -*- coding:utf-8 -*-
#!/usr/bin/python
import g2p_en
import io
from string import digits

remove_digits = str.maketrans('', '', digits)
g2p=g2p_en.G2p()
txt_path = './general_label_en.txt'

with io.open(txt_path, "r") as f:
    for row in f:
        row = row.strip("\n")

        phonemes=g2p(row)
        if "'" in phonemes:
            continue
        out_item = ' '.join(phonemes)
        out_item = out_item.translate(remove_digits)
        out_item = out_item.replace('  ', '')
        print(out_item)