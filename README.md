# untyped_lambda

## 概要
untyped_lambdaはC++によって記述された，ラムダ計算のインタプリタです．

## プログラムの使用方法
untyped_lambdaを実行するには，式の書かれたファイルのパスを以下のようにコマンドラインから引数として渡します．

- untyped_lambda `inputfile`

このとき，以下のオプションを後続して渡すことで動作を切り替えられます．

- `-h` : ヘルプメッセージを表示する．  
- `-o` : 式の評価結果のみ表示する．
- `-b` : 式の評価結果の前に値を表示する．  
- `-s` : 式の評価ごとに一時停止する．任意のキーを押下することで再開．

## 式の記述方法
## コメント
`(* ... *)`で囲むことによって，ソースコードの任意の位置にコメントを書き込むことができます．  
コメントは空白と同じ扱いになります．

## 評価対象の式
式はピリオド`.`で区切られます．以下は`x`と`y z`と`u v w`の式です．  
空白は字句解析時に無視されます．  

```
x.  
y z.  
u v w.
```

## ラムダ式
例えば引数xを取りxを返すラムダ式を記述するには`/x. x.`とします．  
複数の引数を取る場合は`/x y z. ...`と記述するか，1引数のラムダを連ねてください．  

```
(* λx. ... *)
/x. ...
(* λx. λy. λz. ... *)
/x y z. ...
/x. /y. /z. ...
```

### 代入式
左辺トークンに`=`で右辺を設定することで，代入式として扱うことができます．  
代入式そのものは評価されません．
代入式の左辺に用いられたトークンは，以降別の式で記述されるたびに右辺の式として評価されます．

```
0 = /f x. x.
1 = /f x. f x.
2 = /f x. f (f x).
3 = /f x. f (f (f x)).
add = /m n. m succ n.
(* ... *)
add 3 3.
```

```
-> /f x. f (f (f (f (f (f x))))).
```


