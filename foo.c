
void main(){
  int x = 0;
  const (int *)y = &x;
  y = &x;
}
