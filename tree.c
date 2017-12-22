#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


struct record {			/* データレコード */
  char *key;				/* 一次インデクスキー */
  char *col1, *col2,*col3;			/* 二次インデクスキー */
};

struct cell {			/* インデクス */
  char *key;				/* インデクスキー */
  struct record *rec;			/* データレコード */
  struct cell   *left;			/* 部分木 */
  struct cell   *right;			/* 部分木 */
};

struct cell *pindex;		/* 一次インデクスの根 */
struct cell *index1;		/* 二次インデクスの根 */
struct cell *index2;		/* 二次インデクスの根 */

int verbose;


void dump( struct cell *tree, int rev ) {	/* 逆順出力可能  */
  struct record *p;

  if ( !rev && ( tree->left  != NULL ) ) { dump( tree->left,  rev ); }
  if (  rev && ( tree->right != NULL ) ) { dump( tree->right, rev ); }
  p = tree->rec;
  fprintf( stdout, "%s\t%s\t%s\t%s\n", p->key, p->col1, p->col2,p->col3 );
  if ( !rev && ( tree->right != NULL ) ) { dump( tree->right, rev ); }
  if (  rev && ( tree->left  != NULL ) ) { dump( tree->left,  rev ); }
}

void search( char *key, struct cell *tree ) {
  struct record *p;

  if ( tree != NULL ) {
    if ( strcmp( key, tree->key ) == 0 ) {		/* キー一致 */
      p = tree->rec;
      fprintf( stdout, "%s\t%s\t%s\n", p->key, p->col1, p->col2 );
      search( key, tree->left );			/* 同じキーの残りを左部分木で探す */
    } else if ( strcmp( key, tree->key ) < 0 ) {
      search( key, tree->left );
    } else {
      search( key, tree->right );
    }
  }
}


struct record *insert( char *key, struct record *rec, struct cell **tree ) {	/* 挿入, キー重複禁止 */
  struct cell   *p;
  
  if ( *tree == NULL ) {				/* 空いた場所発見 */
    *tree = p = (struct cell *) malloc( sizeof( struct cell ) ); 
    p->key = key;
    p->rec = rec;
    p->left = p->right = NULL;
    return rec;							/****/			
  } else {
    if ( strcmp( key, (*tree)->key ) == 0 ) {		/* 重複キーのエラー処理 */
      fprintf( stderr, "key: \"%s\" already exists ...\n", key );
      return NULL;
    } else if ( strcmp( key, (*tree)->key ) < 0 ) {
      return insert( key, rec, &((*tree)->left) );
    } else {
      return insert( key, rec, &((*tree)->right) );
    }
  }
}

void sindex( char *key, struct record *rec, struct cell **tree ) {	/* 挿入, キー重複可能 */
  struct cell *p;
  
  if ( *tree == NULL ) {				/* 空いた場所発見 */
    *tree = p = (struct cell *) malloc( sizeof( struct cell ) ); 
    p->key = key;
    p->rec = rec;
    p->left = p->right = NULL;
  } else {
    if ( strcmp( key, (*tree)->key ) == 0 ) {		/* 重複キーは左の部分木へ */
      sindex( key, rec, &((*tree)->left) );
    } else if ( strcmp( key, (*tree)->key ) < 0 ) {
      sindex( key, rec, &((*tree)->left) );
    } else {
      sindex( key, rec, &((*tree)->right) );
    }
  }
}


void intern( char *key, char *c1, char *c2 ,char *c3) {		/* データ入力 */
  struct record *rec;
  
  rec = (struct record *) malloc( sizeof( struct record ) );	/* 新規レコード */
  rec->key  = key;
  rec->col1 = c1;
  rec->col2 = c2;
  rec->col3 = c3;

  if ( insert( key, rec, &pindex ) ) {
    sindex( c1,  rec, &index1 );
    sindex( c2,  rec, &index2 );
  	sindex( c3,  rec, &index2);
  } else {
    free( rec );
  }
}


void main( int argc, char **argv ) {
  int  key, rev;
  char ch, str[256], copyright[] = "(c) Copyright M. Takata, 2012";

  pindex = index1 = index2 = NULL;
  key = rev = verbose = 0; str[0] = '\0';

  while ((ch = getopt(argc,argv,"vhrk:s:")) != -1) {
    if ( ch == 'v' ) { verbose = 1; }
    if ( ch == 'h' ) { fprintf( stderr, "columns [-vhr] [-k (0|1|2)] [-s key]\n%s\n", copyright ); exit(1); }
    if ( ch == 'r' ) { rev = 1; }					/* ソート順 */
    if ( ch == 'k' ) { key = atoi( optarg ); }				/* 検索対象とするインデクス */
    if ( ch == 's' ) { strncpy( str, optarg, 255 ); str[255] = '\0'; }	/* 検索するキー文字列 */
  }

  intern( "ichi",   "1", "one","a"   );
  intern( "ni",     "2", "two","b"   );
  intern( "san",    "3", "three","c" );
  intern( "yon",    "4", "four","d"  );
  intern( "shi",    "4", "four","e"  );
  intern( "go",     "5", "five","f"  );
  intern( "roku",   "6", "six"  ,"g" );
  intern( "nana",   "7", "seven","h" );
  intern( "shichi", "7", "seven","i" );
  intern( "hachi",  "8", "eight","j" );

  if ( !str[0] && key == 0 ) dump( pindex, rev );
  if ( !str[0] && key == 1 ) dump( index1, rev );
  if ( !str[0] && key == 2 ) dump( index2, rev );
  if (  str[0] && key == 0 ) search( str, pindex );
  if (  str[0] && key == 1 ) search( str, index1 );
  if (  str[0] && key == 2 ) search( str, index2 );
}