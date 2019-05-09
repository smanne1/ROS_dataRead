function [dx, y] = dcmotor_right(t, x, u, Kt, Kdel, Bdel, varargin)
%DCMOTOR_M  ODE file representing the dynamics of a motor.
% The same DC-motor that was modeled by IDGREY; see IDDEMO7.

%   Copyright 2005-2014 The MathWorks, Inc.

% Output equations.
y = [x(1)]; % Motor Torque


% State equations.
dx = [(-5.8*10^5)*x(1)+Kt*((-4.09*10^5)*x(2)+(4.09*10^5)*u(1));  ... % Motor Torque Kt = Kt*4.09*10^5
      (x(1)*Kdel/(9.68))+(-Bdel)*x(2) ];        % back EMF ];  % Kdel = Kb/Iw % Bdel = B/Iw
  