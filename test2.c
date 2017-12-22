#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define DataLeng	(127)	/* データレコードの文字列の最大長さ */

struct record {			/* データレコード領域 */
  char *key;				/* 一次インデクスのキー */
  char *col1, *col2;			/* 二次インデクスのキー */
  char *rest;				/* インデクスされない領域 */
};

struct cell {			/* インデクスツリーのノード */
  char *key;				/* インデクスのキー */
  struct record *rec;			/* データ領域へのポインタ */
  struct cell   *left;			/* キーが小さい方のサブツリー */
  struct cell   *right;			/* キーが大きい方のサブツリー */
};

struct cell *pindex;		/* 一次インデクスの根 */
struct cell *index1;		/* 二次インデクスの根, その１*/
struct cell *index2;		/* 二次インデクスの根, その２*/


struct record *((*bal_buf)[]);	/* 平衡二分木の作成作業領域: record の保管 */
char          *((*bal_key)[]);	/* 平衡二分木の作成作業領域: key の保管 */
int              bal_cnt;	/* 平衡二分木の作成作業領域: カウンタ */


int records, verbose;		/* 管理用変数 */


/** データ出力用関数 **/

void dump( FILE *fd, struct cell *tree, int rev, int c ) {	/* レコードをすべて書き出す */
  struct record *p;

  if ( !rev && tree->left  ) dump( fd, tree->left,  rev, c ? c+3 : 0 );
  if (  rev && tree->right ) dump( fd, tree->right, rev, c ? c+4 : 0 );
  p = tree->rec;
  fprintf( fd, "%*s%s\t%s\t%s\t%s\n", c, "", p->key, p->col1, p->col2, p->rest );
  if ( !rev && tree->right ) dump( fd, tree->right, rev, c ? c+4 : 0 );
  if (  rev && tree->left  ) dump( fd, tree->left,  rev, c ? c+3 : 0 );
}

struct record *search( FILE *fd, char *from, char *to, struct cell *tree ) {	/* 範囲検索印刷, 逆転不可 */
  struct record *p, *q;

  if ( tree == NULL ) return NULL;
  q = NULL;
  if ( strcmp( from, tree->key ) <=  0  &&  tree->left ) {
    p = search( fd, from, to, tree->left );
    if ( !q ) q = p;
  }
  if ( strcmp( from, tree->key ) <= 0  && strcmp( tree->key, to ) <= 0 ) { 
    p = tree->rec;
    if (fd) fprintf( fd, "%s\t%s\t%s\t%s\n", p->key, p->col1, p->col2, p->rest );
    if ( !q ) q = p;
  }
  if ( strcmp( tree->key, to ) <  0  &&  tree->right ) {
    p = search( fd, from, to, tree->right );
    if ( !q ) q = p;
  }
  return q;
}


/** データ挿入用関数 **/

struct record *insert( char *key, struct record *rec, struct cell **tree ) { /* 一次インデクス挿入・重複禁止 */
  struct cell   *p;
  
  if ( *tree == NULL ) {
    *tree = p = (struct cell *) malloc( sizeof( struct cell ) ); 
    p->key = key;
    p->rec = rec;
    p->left = p->right = NULL;
    return rec;
  } else {
    if ( !strcmp( key, (*tree)->key ) ) {
      fprintf( stderr, "key \"%s\" already exists ...\n", key );
      return NULL;
    } else if ( strcmp( key, (*tree)->key ) < 0 ) {
      return insert( key, rec, &((*tree)->left) );
    } else {
      return insert( key, rec, &((*tree)->right) );
    }
  }
}

void sindex( char *key, struct record *rec, struct cell **tree ) { /* 二次インデクス挿入・重複可能 */
  struct cell *p;
  
  if ( *tree == NULL ) {
    *tree = p = (struct cell *) malloc( sizeof( struct cell ) ); 
    p->key = key;
    p->rec = rec;
    p->left = p->right = NULL;
  } else {
    if ( !strcmp( key, (*tree)->key ) ) {
      sindex( key, rec, &((*tree)->left) );
    } else if ( strcmp( key, (*tree)->key ) < 0 ) {
      sindex( key, rec, &((*tree)->left) );
    } else {
      sindex( key, rec, &((*tree)->right) );
    }
  }
}

struct record *intern( char *key, char *c1, char *c2, char *rest ) {	/* 新しいレコードの挿入 */
  struct record *rec;
  
  rec = (struct record *) malloc( sizeof( struct record ) );
  rec->key  = key;
  rec->col1 = c1;
  rec->col2 = c2;
  rec->rest = rest;

  if ( insert( key, rec, &pindex ) ) {
    sindex( c1,  rec, &index1 );
    sindex( c2,  rec, &index2 );
    records ++;
    return rec;
  } else {
    free( rec );
    return NULL;
  }
}


/** 平衡二分木を作るための関数 **/

void bal_scan( struct cell *tree ) {			/* 二分木を線形に変換 */
  if ( tree->left  ) bal_scan( tree->left  );
  (*bal_buf)[bal_cnt  ] = tree->rec;
  (*bal_key)[bal_cnt++] = tree->key;
  if ( tree->right ) bal_scan( tree->right );
}

void bal_free( struct cell **tree ) {			/* 二分木を分解して消去する */
  if ( (*tree)->left  ) bal_free( &((*tree)->left)  );
  if ( (*tree)->right ) bal_free( &((*tree)->right) );
  free( *tree );
}

void bal_build( int from, int to, struct cell **tree ) {  /* 平衡二分木を組み立てる */
  int n;

  if ( to - from > 2 ) {
    n = (to + from) / 2;
    sindex( (*bal_key)[n], (*bal_buf)[n], tree );
    bal_build( from, n, tree );
    bal_build( n+1, to, tree );
  } else {
    sindex( (*bal_key)[to-1], (*bal_buf)[to-1], tree );
    if ( to - from > 1 )
      sindex( (*bal_key)[to-2], (*bal_buf)[to-2], tree );
  }
}

void balance( int size ) {			/* すべての二分木を平衡にする */
  bal_buf = (struct record *((*)[])) calloc( size, sizeof( struct record * ) );
  bal_key = (char          *((*)[])) calloc( size, sizeof( char * ) );

  bal_cnt = 0;
  bal_scan( index1 );
  bal_free( &index1 );
  if (verbose) fprintf( stderr, "size: %d, x1: %d, ", size, bal_cnt );
  bal_build( 0, bal_cnt, &index1 );

  bal_cnt = 0;
  bal_scan( index2 );
  bal_free( &index2 );
  if (verbose) fprintf( stderr, "x2: %d, ", bal_cnt );
  bal_build( 0, bal_cnt, &index2 );

  bal_cnt = 0;
  bal_scan( pindex );
  bal_free( &pindex );
  if (verbose) fprintf( stderr, "px: %d\n", bal_cnt );
  bal_build( 0, bal_cnt, &pindex );

  free( bal_buf );
  free( bal_key );
}


/** データ削除用関数 **/

struct cell *find_max( struct cell *tree ) {	/* サブツリー内で最大の要素を見つける */
  if ( tree->right ) {
    return find_max( tree->right );
  } else {
    return tree;
  }
}

void remove_cell( char *key, struct record *rec, struct cell **tree ) {	/* ツリー内の要素を削除 */
  struct cell *p;

  if ( *tree == NULL ) return;			/* ポインタが無かったら終了 */
  if ( !strcmp( key, (*tree)->key ) ) {		/* 削除するキーがある */
    if ( (*tree)->rec == rec ) {		/* 削除すべきレコードがある */
      if ( (*tree)->left && (*tree)->right ) {	/* 部分木が二つあるとき */
	p = find_max( ((*tree)->left) );
	(*tree)->key = p->key;
	remove_cell( p->key, rec, &((*tree)->left) );
      } else if ( (*tree)->left ) {		/* 部分木は左側だけ */
	p = (*tree)->left;
	free( *tree );
	*tree = p;
      } else {					/* 部分木はあったとしても右側だけ */
	p = (*tree)->right;
	free( *tree );
	*tree = p;
      }
    }
    if ( *tree && (*tree)->left )		/* 同じキーの違うレコードを処理 */
      remove_cell( key, rec, &((*tree)->left) );
  } else if ( strcmp( key, (*tree)->key ) < 0 ) {
    remove_cell( key, rec, &((*tree)->left) );
  } else {
    remove_cell( key, rec, &((*tree)->right) );
  }
}

void delete( char *key ) {		/* あるレコードをすべてのインデクスから削除する */
  struct record *rec;

  rec = search( NULL, key, key, pindex );
  if ( ! rec ) return; 

  remove_cell( rec->col1, rec, &index1 );
  remove_cell( rec->col2, rec, &index2 );
  remove_cell( rec->key,  rec, &pindex );
  free( rec );
  records--;

}


/** ユーザ・インタフェース **/

char *readfile( FILE *fd ) {		/* ファイルなどからデータを読み込む */
  char *buff, *p, *k, *c1, *c2, *r;

  if (fd == stdin) fprintf( stderr, "RECORD>\t" );

  buff = (char *) malloc( DataLeng+1 );		/* レコード文字列用のメモリ確保 */
  p = k = fgets( buff, DataLeng+1, fd );
  if ( ! k ) return k;
  if ( *k == '\n' ) return k;
  if ( *k == '#' ) return k;		/* コメント行のスキップ */
  if ( *k == '!' ) return k;		/* コメント行のスキップ */
  if ( *k == ';' ) return k;		/* コメント行のスキップ */

  while (*p && *p!='\n') p++;  *p++ = '\0';  *p = '\0';  p = k;			/* 行末 */
  while (*p && *p!='\t') p++;  while (*p && *p=='\t') *p++='\0';  c1 = p;	/* 最初の一連の TAB */
  while (*p && *p!='\t') p++;  while (*p && *p=='\t') *p++='\0';  c2 = p;	/* 次の一連の TAB */
  while (*p && *p!='\t') p++;  while (*p && *p=='\t') *p++='\0';  r = p;	/* 次の一連の TAB */
  if (verbose) fprintf( stderr, "key: %s\n1:\t%s\n2:\t%s\n3:\t%s\n\n", k, c1, c2, r );

  intern( k, c1, c2, r );
  return k;
}


void main( int argc, char **argv ) {	/* メイン関数: ユーザ・インタフェース */
  int  key, rev;
  FILE *fd, *fdo;
  char ch, str[256], copyright[] = "(c) Copyright M. Takata, 2012";
  char line[256], *p, *p1, *p2, *p3;

  pindex = index1 = index2 = NULL;
  key = rev = verbose = records = 0; str[0] = '\0';

  while ((ch = getopt(argc,argv,"vhrk:s:")) != -1) {	/* オプション処理 */
    if ( ch == 'v' ) { verbose = 1; }
    if ( ch == 'h' ) { fprintf( stderr, "cards\n%s\n", copyright ); exit(1); }
    if ( ch == 'r' ) { rev = 1; }					/* 昇順・降順 */
    if ( ch == 'k' ) { key = atoi( optarg ); }				/* インデクスの切り替え */
    if ( ch == 's' ) { strncpy( str, optarg, 255 ); str[255] = '\0'; }	/* 探すキー */
  }

  for ( ; optind < argc; optind++ ) {	/* ファイルからの読み込み */
    fd = fopen( argv[optind], "r" );
    while ( readfile(fd) );
    fclose( fd );
  }

  balance( records+2 );		/* 二分木の平衡化 */
  fprintf( stderr, "%d records loaded\n", records );


  while( fprintf(stderr, "DB> "), p = p1 = fgets( line, 256, stdin ) ) {	/* 会話処理 */
    while (*p && *p!='\n') p++;  *p++ = '\0';  *p = '\0';   p = p1;		/* 行末を NULL に */
    while (*p && !isspace(*p)) p++;  while (*p && isspace(*p)) *p++='\0';  p2 = p;	/* 最初の単語末 */
    while (*p && !isspace(*p)) p++;  while (*p && isspace(*p)) *p++='\0';  p3 = p;	/* 次の単語末 */

    if ( !strcmp( p1, "search" ) ) { 
      if ( key == 0 ) search( stdout, p2, p3, pindex );
      if ( key == 1 ) search( stdout, p2, p3, index1 );
      if ( key == 2 ) search( stdout, p2, p3, index2 );
    }
    if ( !strcmp( p1, "dump" ) ) { 
      if ( key == 0 )  dump( stdout, pindex, rev, 0 );
      if ( key == 1 )  dump( stdout, index1, rev, 0 );
      if ( key == 2 )  dump( stdout, index2, rev, 0 );
    }
    if ( !strcmp( p1, "indent" ) ) { 
      if ( key == 0 )  dump( stdout, pindex, rev, 2 );
      if ( key == 1 )  dump( stdout, index1, rev, 2 );
      if ( key == 2 )  dump( stdout, index2, rev, 2 );
    }
    if ( !strcmp( p1, "insert" ) ) { 
      while ( *readfile( stdin ) != '\n' );
      fprintf( stderr, "%d records loaded\n", records );
    }
    if ( !strcmp( p1, "delete" ) ) {
      delete( p2 );
      fprintf( stderr, "%d records loaded\n", records );
    }
    if ( !strcmp( p1, "save" ) ) {
      if ( *p2 ) {
	if ( fdo = fopen( p2, "w" ) ) {
	  dump( fdo, pindex, 0, 0 );
	  fclose( fdo );
	  fprintf( stderr, "%d records saved into file: %s\n", records, p2 );
	} else 
	  fprintf( stderr, "File: %s open failed\n", p2 );
      } else 
	fprintf( stderr, "Missing file name\n", p2 );
    }

    if ( !strcmp( p1, "key" ) )		key = atoi( p2 );
    if ( !strcmp( p1, "reverse" ) )	rev = atoi( p2 );
    if ( !strcmp( p1, "verbose" ) )	verbose = atoi( p2 );
    if ( !strcmp( p1, "balance" ) )	balance( records+2 );
  }

  fprintf( stderr, "\n" );

}