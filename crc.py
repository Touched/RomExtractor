#!/usr/bin/python3

'''
Generates a CRC32 Table in C array format.
'''

import textwrap

def generateTable(polynomial=0xedb88320):
    '''
    Returns a python list containing a CRC32 code for the range 0x0 - 0xFF
    using the given polynomial.
    '''

    table = []
    for i in range(256):
        rem = i
        for j in range(8):
            if rem & 1:
                rem >>= 1
                rem ^= polynomial
            else:
                rem >>=1 
                
        table.append(rem)
    return table
    
def generateCTable(varname, ctype='uint32_t', polynomial=0xedb88320):
    table = generateTable()
    joined = ', '.join('0x{:08x}'.format(i) for i in table)
    joined = '\t' + '\n\t'.join(textwrap.wrap(joined, 80))
    out = 'static const {} {}[] = {{\n{}\n}};'.format(ctype, varname, joined)
    
    
    return out
    
o = generateCTable('crc32_table')
print(o)
    
    
    
