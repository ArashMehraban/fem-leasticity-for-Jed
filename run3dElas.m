%Solving 3D Elasticity problem using FEM with unstructured mesh
clear
clc
format short

% INSTRUCTIONS:
%   1) Run the code to see it converges. (1176 unstructured hex elements in 0.49 seconds.)
%   Then
%   2) Uncomment %filename = 'cube_1e_6ns_s'; (line 20) and run:
%       (This will run with 1 elem. grad_u values for 1 element based on the  
%        following manufatured solution is displayed):
%         given_u{1}=@(x,y,z)exp(2*x).*sin(3*y).*cos(4*z);
%         given_u{2}=@(x,y,z)exp(3*y).*sin(4*z).*cos(2*x); 
%         given_u{3}=@(x,y,z)exp(4*z).*sin(2*x).*cos(3*y);
%

one_elem_flag = 0;
filename = 'cylinder8_1176e_us';

%filename = 'cube_1e_6ns_s';

if(strcmp(filename, 'cube_1e_6ns_s') == 1)
    one_elem_flag = 1;
end

[~, msh] = get_mesh(filename,'exo','lex');
dof = 3; % linear 3D elasticity

ploy_order = 1;
P = ploy_order + 1; 
userf=@userf_3d_elas;     % used in residual evaluation
userdf=@userdf_3d_elas;   % used in action of Jacobian on delta_u

%gmres setup:
max_iter_gmres = 80;
global_res_tol = 1.0e-8;

%get all Dirichlet boundary node sets
dir_bndry_nodes = get_all_dir_ns(msh);
    
%NOTE: Proposed solution to manufacture rhs from in userf function:
given_u{1}=@(x,y,z)exp(2*x).*sin(3*y).*cos(4*z);
given_u{2}=@(x,y,z)exp(3*y).*sin(4*z).*cos(2*x); 
given_u{3}=@(x,y,z)exp(4*z).*sin(2*x).*cos(3*y);

[dir_bndry_val, exactSol] = get_exact_sol(msh.vtx_coords,dir_bndry_nodes, given_u);

if(msh.num_dims ==3)
   if(msh.num_nodes_per_elem == 8)
       elemType = 'Hex8';
   end
  
end
elemTypeMsg = ['Mesh element type: ', elemType];
disp(elemTypeMsg);
numElmMsg = ['Number of elements in mesh: ',num2str(msh.num_elem)];
disp(numElmMsg);

fem_sol =  get_fem_sol(msh, dof, dir_bndry_nodes,dir_bndry_val,P,userf,userdf,max_iter_gmres,global_res_tol,one_elem_flag);

error = norm(exactSol - fem_sol)/norm(fem_sol);
L2ErrMsg = strcat('L2 Error: ', num2str(error));
disp(L2ErrMsg);

