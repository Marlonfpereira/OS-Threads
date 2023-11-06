import subprocess 
import time


for _ in range(1):

    # Obtenha o tempo de início
    tempo_inicial = time.time()
    
    result = subprocess.run(["time", "-p", "./restaurant-serial.x", "20"] , capture_output=True).stderr
    # subprocess.Popen(["./restaurant.x"] + ["25","5","3"]).wait()
    result = result.decode("utf-8")
    # Obtenha o tempo de término
    tempo_final = time.time()

    # Calcule o tempo decorrido
    tempo_decorrido = tempo_final - tempo_inicial

    real_time = float(result.split("user")[0].split("real")[1])
    user_time = float(result.split("sys")[0].split("user")[1])
    sys_time = float(result.split("sys")[1])

    
    with open("results-serial.txt", "a+") as result_file:
        result_file.write(f"{real_time},{user_time},{sys_time}\n")



for _ in range(1):

    # Obtenha o tempo de início
    tempo_inicial = time.time()
    
    result = subprocess.run(["time", "-p", "./restaurant.x", "20", "4", "6"] , capture_output=True).stderr
    # subprocess.Popen(["./restaurant.x"] + ["25","5","3"]).wait()
    result = result.decode("utf-8")
    # Obtenha o tempo de término
    tempo_final = time.time()

    # Calcule o tempo decorrido
    tempo_decorrido = tempo_final - tempo_inicial

    real_time = float(result.split("user")[0].split("real")[1])
    user_time = float(result.split("sys")[0].split("user")[1])
    sys_time = float(result.split("sys")[1])

    with open("results-paralelo.txt", "a+") as result_file:
        result_file.write(f"{real_time},{user_time},{sys_time}\n")


