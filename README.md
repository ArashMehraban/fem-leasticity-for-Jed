## Instructions on how to run the code:

You may downlaod the code and run it. It will use 1176 unstructured `Hex8` elements and converges about 1.4 seconds with L2 error of 0.030.\
Then\
Uncomment `%filename = 'cube_1e_6ns_s';` (line 20) and run:\
(This will run with 1 element. grad_u values for 1 element based on the following manufatured solution is displayed):\
```
         given_u{1}=@(x,y,z)exp(2*x).*sin(3*y).*cos(4*z);
         given_u{2}=@(x,y,z)exp(3*y).*sin(4*z).*cos(2*x); 
         given_u{3}=@(x,y,z)exp(4*z).*sin(2*x).*cos(3*y);
```


**Note**:\
You may check those values against the output for 1 element from our CEED/PETSc code for grad_u to verify they are the same.

 

