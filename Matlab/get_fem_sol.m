function u = get_fem_sol(msh, dof, dir_bndry_nodes,dir_bndry_val,P,userf,userdf,max_iter_gmres,global_res_tol,one_elem_flag)
%GET_FEM_SOL returns the solution to the PDE
%
%input :                msh: mesh object
%      :         sz_u_field: size of unknown field (eg. : Poisson 1, Plane Strain 2)
%      :    dir_bndry_nodes: Dirichlet Boundary nodes
%      :      dir_bndry_val: Dirichlet Boundary values
%      :num_quadr_pts_in_1d: number of quadrature points in 1 dimension
%      :              userf: any knowns physics resources as a function input for global residual 
%      :             userdf: any knowns physics resources as a function inout for consistent tangent 
%      :     max_iter_gmres: user specified maximum number of itertions before GMRES stops     
%      :        max_iter_nw: user specified maximum number newton steps before Newton stops
%      :     global_res_tol: user specified tolerance for norm of global_res before solver stops  
%
%output:              u: FEM Solution u
%           gl_res_norm: An array where each element contains the global residual norm after an iteration of GMRES
%               JACOB__: Jacobian of the system returned by the solver
%
% NOTE: return values that end with __ (e.g. JACOB__) are not intended to
% be used in the program per se. Be very careful before using them. 

    
    %get number of nodes in mesh
    num_nodes = msh.num_nodes;  
    
    %get the unknown u guess vector and global to local mapping for each u
    [u, global_idx_map] = get_global_map(num_nodes,dir_bndry_nodes,dof);
    
    tic           
    global_jac = get_global_jac(global_idx_map, msh,P,dof, userdf);            
    elapsedTime=toc;
    msg = strcat('global Jacobian matrix assembly:', num2str(elapsedTime), ' seconds');
    if (one_elem_flag == 0)
       disp(msg);
    end
     
    iter=1;
        while(true)

            global_res = get_global_res(u, global_idx_map, msh, dir_bndry_val, P, userf,one_elem_flag); 

            global_res_norm = norm(global_res,inf);
            if(global_res_norm < global_res_tol)
               break;
            end  

            jv_fun = @(dlta_u)get_global_Jv(dlta_u, global_idx_map, msh, P, userdf);

            precond_jac_fun = @(u)(global_jac\u);
            tic
            dlta_u= gmres(jv_fun, global_res,max_iter_gmres,1e-6,[],precond_jac_fun);
            elapsedTime = toc;
            msg = strcat('gmres solve time: ', num2str(elapsedTime), ' seconds');
            disp(msg);
            u=u-dlta_u;
            
            iter=iter+1;
        end
end

