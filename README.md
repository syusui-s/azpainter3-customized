# AzPainter

http://azsky2.html.xdomain.jp/

イラストや画像編集用のペイントソフト。

## 動作環境

- Linux/FreeBSD/macOS(要XQuartz)
- X11R6 以降

## コンパイル/インストール

※各ライブラリの開発用ファイルが必要です。

~~~
## Linux

$ ./configure
$ make
$ sudo make install

## FreeBSD

$ ./configure
$ gmake
# gmake install

## macOS

$ ./configure --prefix=/opt/X11
$ make
$ sudo make install
~~~
