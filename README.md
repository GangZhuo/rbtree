# RB-Tree

RB-Tree implementation.

## Usage for test
```
$ ./rbtree
Red-Black-Tree
  commands:
    insert <integer>       insert integer value(s),
                           the value should be positive.
    delete <integer>       delete value(s).
    find   <integer>       find node(s).
    clear                  clear red-black-tree.
    print  [pre|in|post]   print red-black-tree.
                             options:
                               pre  - preorder.
                               in   - inorder.
                               post - postorder.
    load   <path>          load from file.
    save   <path>          save to file.
    bmp    [nonil] <path>  save as bitmap.
    help                   print help.
    quit                   quit.
```

## Generate image
```
$ ./rbtree insert 1 insert 2 insert 3 insert 4 insert 5 insert 6 insert 7 insert 8 insert 9 bmp 1.bmp quit
insert 1.
insert 2.
insert 3.
insert 4.
insert 5.
insert 6.
insert 7.
insert 8.
insert 9.
Creating bitmap...
Done. 9 entries.
```

## Example

* [example1]
* [example2]


[example1]: https://github.com/GangZhuo/rbtree/blob/master/example/example1.c
[example2]: https://github.com/GangZhuo/rbtree/blob/master/example/example2.c
