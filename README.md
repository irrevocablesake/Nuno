# Nuno

Nuno is a cloth simulation based on extended position based dynamics, a different approach to physics based sims. Based on the Research Papers by Matthias Müller-Fischer & Team at NVIDIA. 

The architecture is made using "**passes**", each pass is either a "**compute shader**" or a "**graphics shader**". In each pass, we calculate some information and pass forward to other passes. Overall the algorithm looks as follows:
- Predict the positions
- Correct the prediction using various constraints:
    - **Structural Constraints**: Maintains the Horizontal and Vertical structure
    - **Shear Constraints**: Maintains the Diagonal shape of the cloth 
    - **Bend Constraints**: Ensures that the cloth doesn't bend a lot
    - **Area Constraints**: Ensures that the area after the above constraints is correct and helps us to construct normals
- Apply the correction and guidelines for further simulation

Note: The above algorithm is solved using two types of solvers: **Jacobi** vs **Gauss Seidel**. As of now, all the above ones are implemented using Jacobi, a self-collision constraint will be added which will use a Gauss Seidel Solver. The reason for the combination of solvers is simple - performance, maintaining algorithm according to architecture and in-order to avoid race conditions.

It's still a WIP, and hence this section is empty ( for now ). 

Eventually, I will be updating this page as I progres.

## Video

Please find the video example of how the simulation looks like so far

https://github.com/user-attachments/assets/f31b3443-d2d1-44f3-98a3-4316194200c1

## Resources

1) [ XPBD ]( https://matthias-research.github.io/pages/publications/XPBD.pdf )
