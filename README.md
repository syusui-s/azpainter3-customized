# AzPainter 3

This is unofficial and customized version of AzPainter 3.

* Maybe there are something non-production codes such as debug print and tricky hacks, and it may contains bugs.
* "This program comes with ABSOLUTELY NO WARRANTY;" (GPL 3)

See [commits](https://github.com/syusui-s/azpainter3/commits/master) to check the changes.

---

# AzPainter

http://azsky2.html.xdomain.jp/

イラストや画像編集用のペイントソフト。<br>
8bit/16bit カラーに対応。

## 動作環境

- Linux、macOS(要XQuartz) ほか
- X11R6 以降

## コンパイル/インストール

ninja、pkg-config コマンドが必要です。<br>
各開発用ファイルのパッケージも必要になります。ReadMe を参照してください。

~~~
$ ./configure
$ cd build
$ ninja
# ninja install
~~~
