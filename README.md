# README PLEASE #
This repository is made for generate image from text because I have a problem with indic text rendering SO I use image instead!.

I have no experience with C. All I know it works! but codes are suck. If you have time and expierence with C and rewrite code to be readable, it would be very very appreciate.

# Instruction #
1. git clone https://github.com/rutcreate/harfbuzz-tutorial.git
2. cd harfbuzz-tutorial
3. make
4. chmod +x text2image
5. ./text2image

# Usage #
./text2image &lt;Font file&gt; &lt;Font size&gt; &lt;Line spacing&gt; &lt;Color&gt; &lt;Alignment&gt; &lt;Max width&gt; &lt;Output file&gt; &lt;Text&gt;

### Font file ###
Absolute path or relative path to font file.

### Font size ###
Font size

### Line spacing ###
Spacing between lines. (In case there are more than 1 line.)

### Color ###
It needs to be RGBA hex code seperated by spaces and remove #. Please see example below.
- #FFFFFFFF convert to "FF FF FF FF"
- #00FF3380 convert to "00 FF 33 80"

### Alignment ###
Accept only left, middle, and right. Otherwise, it will fallback to left. It won't work on multiple lines because there is no line to compare.

### Max width ###
Set max width to 0 for unlimited image width. Otherwise, it will be sized by given value.

### Output file ###
Location for output file.

### Text ###
Text to generate.

# Example #

## Single line ##
./text2image NotoSans-Regular.ttf 24 8 "00 00 00 ff" left 0 single_line.png "Quisque ut dolor gravida, placerat libero vel, euismod."

## Multiple lines ##
./text2image NotoSans-Regular.ttf 24 8 "00 00 00 ff" left 0 multiple_lines.png "Quisque ut dolor gravida,
placerat libero vel, euismod."

## Multiple lines by max width ##
./text2image NotoSans-Regular.ttf 24 8 "00 00 00 ff" left 200 multiple_lines_by_max_width.png "Quisque ut dolor gravida, placerat libero vel, euismod."

## Alignment Center ##
./text2image NotoSans-./text2image NotoSans-Regular.ttf 24 8 "00 00 00 ff" center 0 alignment_center.png "Quisque ut dolor gravida,
placerat libero vel, euismod."

## Alignment Right ##
./text2image NotoSans-./text2image NotoSans-Regular.ttf 24 8 "00 00 00 ff" right 0 alignment_right.png "Quisque ut dolor gravida,
placerat libero vel, euismod."
