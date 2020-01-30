## Instructions on how to run the Matlab code:

You may downlaod the code and run it from `run3dElas.m`. It will use 1176 unstructured `Hex8` elements and converges about 1.4 seconds with L2 error of 0.030.\
Then\
Uncomment `%filename = 'cube_1e_6ns_s';` (line 20 in `run3dElas.m`) and run:\
(This will run with 1 element. grad_u values for 1 element based on the following manufatured solution is displayed):
```
         given_u{1}=@(x,y,z)exp(2*x).*sin(3*y).*cos(4*z);
         given_u{2}=@(x,y,z)exp(3*y).*sin(4*z).*cos(2*x); 
         given_u{3}=@(x,y,z)exp(4*z).*sin(2*x).*cos(3*y);
```

**Note**:\
You may check those values against the output for 1 element from our CEED/PETSc code for grad_u to verify they are the same.

**Results from Matlab Code:**

```
Mesh element type: Hex8
Number of elements in mesh: 1
  
grad_ue per Node (lex ordering):
    [du1/dx | du1/dy | du1/dz]
    [du2/dx | du2/dy | du2/dz]
    [du3/dx | du3/dy | du3/dz]
 
    0.5633   -0.7178         0
    0.0000    1.2079    1.1035
    0.6972    0.0000   -0.2493

    0.5633   -0.7178         0
   -0.0000   -1.2079    1.1035
    0.1986   -0.0000   -0.2493

   -0.5633   -0.7178         0
    0.0000    1.2079    3.5194
    0.6972    0.0000   -0.2493

   -0.5633   -0.7178         0
   -0.0000   -1.2079    3.5194
    0.1986   -0.0000   -0.2493

    0.5633   -1.8444         0
    0.0000    1.2079    1.1035
    0.6972   -0.0000    0.2493

    0.5633   -1.8444         0
   -0.0000   -1.2079    1.1035
    0.1986    0.0000    0.2493

   -0.5633   -1.8444         0
    0.0000    1.2079    3.5194
    0.6972   -0.0000    0.2493

   -0.5633   -1.8444         0
   -0.0000   -1.2079    3.5194
    0.1986    0.0000    0.2493
```

**NOTE:** PETSc code uses mesh files that have sidesets (ss in the mesh file name) while Matlab uses nodesets (ns in mesh file name)
```
./elasticity -mesh ./meshes/cube_1e_6ss_s.exo -degree 1 -nu .3 -E 1 -boundary mms -forcing mms -snes_monitor 
L2 Error: -nan
Qpt 0
0.563299	-0.717780	-0.000000	
0.000000	1.207938	1.103518	
0.697157	0.000000	-0.249280	
Qpt 1
0.563299	-0.717780	-0.000000	
-0.000000	-1.207938	1.103518	
0.198597	-0.000000	-0.249280	
Qpt 2
-0.563299	-0.717780	0.000000	
-0.000000	1.207938	3.519395	
0.697157	0.000000	-0.249280	
Qpt 3
-0.563299	-0.717780	0.000000	
0.000000	-1.207938	3.519395	
0.198597	-0.000000	-0.249280	
Qpt 4
0.563299	-1.844378	0.000000	
0.000000	1.207938	1.103518	
0.697157	-0.000000	0.249280	
Qpt 5
0.563299	-1.844378	0.000000	
-0.000000	-1.207938	1.103518	
0.198597	0.000000	0.249280	
Qpt 6
-0.563299	-1.844378	-0.000000	
-0.000000	1.207938	3.519395	
0.697157	-0.000000	0.249280	
Qpt 7
-0.563299	-1.844378	-0.000000	
0.000000	-1.207938	3.519395	
0.198597	0.000000	0.249280
```
 

