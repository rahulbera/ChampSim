import lhsmdu
import numpy as np

samples = 20
k = lhsmdu.sample(3, samples)
k = np.array(k)

for x in range(len(k[0])):
    # print("{0:.3f} {1:.3f} {2:.3f}".format(k[0][x],k[1][x],k[2][x]))
    print("scooby_gs2_a_{0:.3f}_g_{1:.3f}_e_{2:.3f}  $(BASE) $(SCOOBY_CMAC) --scooby_alpha={0:.3f} --scooby_gamma={1:.3f} --scooby_epsilon={2:.3f}".format(k[0][x], k[1][x], k[2][x]))