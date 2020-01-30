% function [f0,f1] = userf_3d_elas(ue, grad_ue, xe,invJe,Du) 
function [f0,f1] = userf_3d_elas(ue, grad_ue, xe, one_elem_flag) 
%USERF_3d_ELAS provides weak form of the linear 3D Elastisity problem to solve 
%
%  input:      ue: corresponding u for each element evalauted at quadrature points
%       : grad_ue: corresponding grad_u for each element evalauted at quadrature points
%       :      xe: quadrature points mapped to the reference elem
%
% output:  f0: any possible source from given problem
%       :  f1: any possible source from given problem



    x=xe(:,1);
    y=xe(:,2);
    z=xe(:,3);
    
   %grad_ue  structure that comes into this function is which is really Transpose of grad_u
   % For linear 3D elasticity we do NOT transpose grad_u since 
   % strain = 0.5(grad_u + grad_u^T)

   
   if(one_elem_flag == 1)
        disp('  ');
        disp('grad_ue per Node (lex ordering):');
        disp('    [du1/dx | du1/dy | du1/dz]');
        disp('    [du2/dx | du2/dy | du2/dz]');
        disp('    [du3/dx | du3/dy | du3/dz]');
        disp(' ');
        for i=1:8
            disp([grad_ue(i,:);grad_ue(i+8,:);grad_ue(i+2*8,:)]')            
        end        
   end
  


    
    % Young's modulus 
    E = 1;%2e11;
    % Poisson ratio 
    nu = 0.3;
        
    num_row = size(ue,1);
    
    %strain-stress matrix coefficient
    ss_coef = E/((1+nu)*(1-2*nu));
    
    %strain
    eps11 = grad_ue(1:num_row,1);
    eps22 = grad_ue(num_row+1:2*num_row,2);
    eps33 = grad_ue(2*num_row+1:end,3);
    eps12 = 0.5*(grad_ue(1:num_row,2)+grad_ue(num_row+1:2*num_row,1));
    eps13 = 0.5*(grad_ue(1:num_row,3)+grad_ue(2*num_row+1:end,1));
    eps23 = 0.5*(grad_ue(num_row+1:2*num_row,3)+grad_ue(2*num_row+1:end,2));
    
    sigma11 = ss_coef*((1-nu)*eps11 + nu*eps22 + nu*eps33);
    sigma22 = ss_coef*(nu*eps11 + (1-nu)*eps22 + nu*eps33);
    sigma33 = ss_coef*(nu*eps11 + nu*eps22 + (1-nu)*eps33);
    sigma12 = ss_coef*(0.5*(1-2*nu)*eps12);    
    sigma13 = ss_coef*(0.5*(1-2*nu)*eps13);  
    sigma23 = ss_coef*(0.5*(1-2*nu)*eps23);
   

    % RHS: manufactured from construct_pln_strn_rhs.m
    g1=(-(E.*(cos(x.*2.0).*cos(y.*3.0).*exp(z.*4.0).*4.0-cos(z.*4.0).*sin(y.*3.0).*exp(x.*2.0).*8.0).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*(cos(z.*4.0).*sin(y.*3.0).*exp(x.*2.0).*(9.0./2.0)+sin(x.*2.0).*sin(z.*4.0).*exp(y.*3.0).*3.0).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*nu.*cos(x.*2.0).*cos(y.*3.0).*exp(z.*4.0).*8.0)./((nu.*2.0-1.0).*(nu+1.0))-(E.*nu.*sin(x.*2.0).*sin(z.*4.0).*exp(y.*3.0).*6.0)./((nu.*2.0-1.0).*(nu+1.0))-(E.*cos(z.*4.0).*sin(y.*3.0).*exp(x.*2.0).*(nu-1.0).*4.0)./((nu.*2.0-1.0).*(nu+1.0)));
    g2=(-(E.*(cos(y.*3.0).*cos(z.*4.0).*exp(x.*2.0).*3.0-cos(x.*2.0).*sin(z.*4.0).*exp(y.*3.0).*2.0).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*(cos(x.*2.0).*sin(z.*4.0).*exp(y.*3.0).*8.0+sin(x.*2.0).*sin(y.*3.0).*exp(z.*4.0).*6.0).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*nu.*cos(y.*3.0).*cos(z.*4.0).*exp(x.*2.0).*6.0)./((nu.*2.0-1.0).*(nu+1.0))-(E.*nu.*sin(x.*2.0).*sin(y.*3.0).*exp(z.*4.0).*1.2e1)./((nu.*2.0-1.0).*(nu+1.0))-(E.*cos(x.*2.0).*sin(z.*4.0).*exp(y.*3.0).*(nu-1.0).*9.0)./((nu.*2.0-1.0).*(nu+1.0)));
    g3=(-(E.*(cos(x.*2.0).*cos(z.*4.0).*exp(y.*3.0).*6.0-cos(y.*3.0).*sin(x.*2.0).*exp(z.*4.0).*(9.0./2.0)).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*(cos(y.*3.0).*sin(x.*2.0).*exp(z.*4.0).*2.0+sin(y.*3.0).*sin(z.*4.0).*exp(x.*2.0).*4.0).*(nu-1.0./2.0))./((nu.*2.0-1.0).*(nu+1.0))+(E.*nu.*cos(x.*2.0).*cos(z.*4.0).*exp(y.*3.0).*1.2e1)./((nu.*2.0-1.0).*(nu+1.0))-(E.*nu.*sin(y.*3.0).*sin(z.*4.0).*exp(x.*2.0).*8.0)./((nu.*2.0-1.0).*(nu+1.0))-(E.*cos(y.*3.0).*sin(x.*2.0).*exp(z.*4.0).*(nu-1.0).*1.6e1)./((nu.*2.0-1.0).*(nu+1.0)));
    
    f0 = [-g1, -g2 , -g3]; 
    f1(:,1) = [sigma11 ; sigma12 ; sigma13];
    f1(:,2) = [sigma12 ; sigma22 ; sigma23]; 
    f1(:,3) = [sigma13 ; sigma23 ; sigma33];
    
end