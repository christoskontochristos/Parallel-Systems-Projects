

Εξήγηση κώδικα:

Η συνάρτηση που είχε το μεγαλύτερο computational cost και έπρεπε να παραλληλοποιηθεί είναι η bodyForce.
Έχει οριστεί ως __global__ , δηλαδή CUDA kernel για να τρέξει στην GPU. Κάθε GPU thread ουσιαστικά κάνει 
τους υπολογισμούς για ένα σώμα (παραλείπουμε τελέιως το εξωτερικό for loop). Αρχικά παίρνουμε το i με τον εξής τύπο : 
int i = blockIdx.x * blockDim.x + threadIdx.x και στη συνέχεια γίνεται ο υπολογισμός. Επειδή κάθε thread
ασχολείται με δικό του ξεχωριστό index (i) δεν υπάρχει ανάγκη για shared memory, επομένως όλα τα reads/writes 
πάνε στο global memory. Επίσης η λειτουργία που κάνει update τη θέση κάθε σώματος μετά από τους υπολογισμούς 
είναι επίσης παραλληλοποιημένη (ξεχωριστός CUDA kernel) έτσι κάθε thread της GPU είναι υπέυθυνο να κάνει update τη 
θέση ενός σώματος. Με τον παραπάνω τρόπο επιταγχύνεται ο n-body simulator, με λίγα λόγια κάθε thread της GPU είναι
υπέυθυνο για τον υπολογισμό ενός σώματος.

Τα blocks per grid υπολογίζονται έτσι (threadsPerBlock αρχικοποιείται σε 256):
int blocksPerGrid = (nBodies + threadsPerBlock - 1) / threadsPerBlock;


CUDA Memory Management:

Πρίν τον υπολογισμό:

Χρησιμοποιείται cudaMalloc() για δέσμευση στη GPU του πίνακα με τα n σώματα.
Χρησιμοποιείται cudaMemcpy() για memory transfer από τη CPU στη GPU.

Μετά τον υπολογισμό:

Χρησιμοποιείται cudaMemcpy() για memory transfer τώρα από τη GPU στη CPU.
Τέλος χρησιμοποιείται cudaFree() για να ελευθερώσει την δεσμευμένη GPU μνήμη.




Τρέχοντας την run_assessment() παίρνουμε τα εξής αποτελέσματα: 

Running nbody simulator with 4096 bodies
----------------------------------------

Application should run faster than 0.9s
Your application ran in: 0.1414s
Your application reports  54.243 Billion Interactions / second

Your results are correct

Running nbody simulator with 65536 bodies
----------------------------------------

Application should run faster than 1.3s
Your application ran in: 0.1904s
Your application reports  419.541 Billion Interactions / second

Your results are correct

Congratulations! You passed the assessment!
See instructions below to generate a certificate, and see if you can accelerate the simulator even more!




Report file:

CUDA API Statistics:

 Time(%)  Total Time (ns)  Num Calls    Average     Minimum    Maximum           Name         
 -------  ---------------  ---------  -----------  ---------  ---------  ---------------------
    98.7        298184779          1  298184779.0  298184779  298184779  cudaMalloc           
     1.0          3054113         20     152705.7       3200     301448  cudaDeviceSynchronize
     0.2           589165         20      29458.3       4160     475797  cudaLaunchKernel     
     0.0           112659          1     112659.0     112659     112659  cudaFree             
     0.0            87749          2      43874.5      34090      53659  cudaMemcpy 

CUDA Kernel Statistics:

 Time(%)  Total Time (ns)  Instances  Average   Minimum  Maximum                  Name                
 -------  ---------------  ---------  --------  -------  -------  ------------------------------------
    99.1          2953350         10  295335.0   294327   297463  bodyForce(Body*, float, int)        
     0.9            25762         10    2576.2     2497     2944  integratePosition(Body*, float, int)

CUDA Memory Operation Statistics (by time):

 Time(%)  Total Time (ns)  Operations  Average  Minimum  Maximum      Operation     
 -------  ---------------  ----------  -------  -------  -------  ------------------
    52.5             9568           1   9568.0     9568     9568  [CUDA memcpy HtoD]
    47.5             8640           1   8640.0     8640     8640  [CUDA memcpy DtoH]

CUDA Memory Operation Statistics (by size in KiB):

 Total   Operations  Average  Minimum  Maximum      Operation     
 ------  ----------  -------  -------  -------  ------------------
 96.000           1   96.000   96.000   96.000  [CUDA memcpy DtoH]
 96.000           1   96.000   96.000   96.000  [CUDA memcpy HtoD]

Operating System Runtime API Statistics:

 Time(%)  Total Time (ns)  Num Calls   Average    Minimum   Maximum        Name     
 -------  ---------------  ---------  ----------  -------  ---------  --------------
    78.8        300415876         14  21458276.9    23020  100126656  poll          
    14.7         55895317        618     90445.5     1000   10379808  ioctl         
     5.0         18911046         25    756441.8     1020   18699707  fopen         
     0.5          1927501         64     30117.2     2810     543417  mmap64        
     0.3          1319182          7    188454.6     1090    1307682  read          
     0.2           926844          9    102982.7    14650     750755  sem_timedwait 
     0.2           857456         76     11282.3     2750      35630  open64        
     0.0           154709          4     38677.3    32270      49680  pthread_create
     0.0           139248         11     12658.9     8180      18689  write         
     0.0           121390         13      9337.7     1430      32160  mmap          
     0.0            59030          1     59030.0    59030      59030  writev        
     0.0            52599         11      4781.7     1130      28450  fclose        
     0.0            48060          1     48060.0    48060      48060  fgets         
     0.0            36920          2     18460.0     8440      28480  fopen64       
     0.0            28860          5      5772.0     3120       8060  open          
     0.0            28120          5      5624.0     1100      14340  fgetc         
     0.0            20270          2     10135.0     8800      11470  fread         
     0.0            20200          3      6733.3     1910      12000  fwrite        
     0.0            17820          5      3564.0     2610       4650  munmap        
     0.0            15390          5      3078.0     1000       7020  fcntl         
     0.0            12700          1     12700.0    12700      12700  sem_wait      
     0.0            10800          2      5400.0     3090       7710  socket        
     0.0             7890          1      7890.0     7890       7890  connect       
     0.0             7749          1      7749.0     7749       7749  pipe2         
     0.0             4710          1      4710.0     4710       4710  fflush        
     0.0             1720          1      1720.0     1720       1720  bind          