 #include<stdio.h>

int a[150][150],reach[150];
int n = 150;
void dfs(int v)
{
 int i;
 reach[v]=1;
 for(i=1;i<=n;i++)
  if(a[v][i] && !reach[i])
  {
   printf("n %d->%d",v,i);
   dfs(i);
  }
}
int main()
{
 int i,j,count=0;
 for(i=1;i<=n;i++)
 {
  reach[i]=0;
  for(j=1;j<=n;j++)
   a[i][j]=0;
 }
 for(i=1;i<=n;i++)
  for(j=1;j<=n;j++)
   a[i][j] = 1;
 dfs(1);
 printf("n");
 for(i=1;i<=n;i++)
 {
  if(reach[i])
   count++;
 }
 if(count==n)
  printf("n Graph is connected");
 else
  printf("n Graph is not connected");
return 0;
}
