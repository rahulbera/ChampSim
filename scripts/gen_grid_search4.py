import numpy as np

min_alpha = 0.005
max_alpha = 0.75
min_gamma = 0.5
max_gamma = 0.75
min_epsilon = 0.001
max_epsilon = 0.3

alpha_list = np.geomspace(min_alpha, max_alpha, num=20)
gamma_list = np.geomspace(min_gamma, max_gamma, num=20)
epsilon_list = np.geomspace(min_epsilon, max_epsilon, num=20)

for alpha in alpha_list:
    for gamma in gamma_list:
        for epsilon in epsilon_list:
            print("scooby_gs_a_{}_g_{}_e_{}  $(BASE) $(SCOOBY) --scooby_alpha={} --scooby_gamma={} --scooby_epsilon={}".format(alpha, gamma, epsilon, alpha, gamma, epsilon))
