#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    BEAR,
    BIRD,
    PANDA
} AnimalType;

typedef enum {
    ALIVE,
    DEAD
} AnimalStatus;

typedef struct {
    int x;
    int y;
} Location;

typedef enum {
    FEEDING,
    NESTING,
    WINTERING
} SiteType;

typedef struct {
    AnimalStatus status;
    AnimalType type;
    Location location;
} Animal;

typedef struct {
    int points;
    Location location;
} Hunter;

typedef struct {
    Hunter **hunters;
    int nhunters;
    Animal **animals;
    int nanimals;
    SiteType type;
} Site;

typedef struct {
    int xlength;
    int ylength;
    Site **sites;
} Grid;

Grid grid = {0, 0, NULL};
pthread_t animal_threads[3];
pthread_t *hunter_threads;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *simulateanimal(void *args);
void *simulatehunter(void *args);

Grid initgrid(int xlength, int ylength)
{
    grid.xlength = xlength;
    grid.ylength = ylength;

    grid.sites = (Site **)malloc(sizeof(Site *) * xlength);
    for (int i = 0; i < xlength; i++)
    {
        grid.sites[i] = (Site *)malloc(sizeof(Site) * ylength);
        for (int j = 0; j < ylength; j++)
        {
            grid.sites[i][j].animals = NULL;
            grid.sites[i][j].hunters = NULL;
            grid.sites[i][j].nhunters = 0;
            grid.sites[i][j].nanimals = 0;
            double r = rand() / (double)RAND_MAX;
            SiteType st;
            if (r < 0.33)
                st = WINTERING;
            else if (r < 0.66)
                st = FEEDING;
            else
                st = NESTING;
            grid.sites[i][j].type = st;
        }
    }

    return grid;
}

void deletegrid()
{
    for (int i = 0; i < grid.xlength; i++)
    {
        free(grid.sites[i]);
    }

    free(grid.sites);

    grid.sites = NULL;
    grid.xlength = -1;
    grid.ylength = -1;
}

void printgrid()
{
    for (int i = 0; i < grid.xlength; i++)
    {
        for (int j = 0; j < grid.ylength; j++)
        {
            Site *site = &grid.sites[i][j];
            int count[3] = {0};
            for (int a = 0; a < site->nanimals; a++)
            {
                Animal *animal = site->animals[a];
                count[animal->type]++;
            }

            printf("|%d-{%d, %d, %d}{%d}|", site->type, count[0], count[1],
                   count[2], site->nhunters);
        }
        printf("\n");
    }
}

void *simulateanimal(void *args)
{
    Animal *animal = (Animal *)args;

    while (1)
    {
        if (animal->status == DEAD)
        {
            printf("Animal died at location (%d, %d)\n", animal->location.x, animal->location.y);
            return NULL;
        }

        int sleepTime = rand() % 3 + 1;
        sleep(sleepTime);

        int dx = rand() % 3 - 1;
        int dy = rand() % 3 - 1;

        Location new_location = {animal->location.x + dx, animal->location.y + dy};

         /*control for is animal in grid*/ 
        if (new_location.x < 0 || new_location.x >= grid.xlength || new_location.y < 0 || new_location.y >= grid.ylength)
            continue;

        Site *current_side = &grid.sites[animal->location.x][animal->location.y];
        Site *new_site = &grid.sites[new_location.x][new_location.y];

        pthread_mutex_lock(&mutex);

        /*remove animal*/  
        for (int i = 0; i < current_side->nanimals; i++)
        {
            if (current_side->animals[i] == animal)
            {
                current_side->animals[i] = current_side->animals[current_side->nanimals - 1];
                current_side->nanimals--;
                break;
            }
        }

        /* Add animal to new site*/
        new_site->animals = (Animal **)realloc(new_site->animals, sizeof(Animal *) * (new_site->nanimals + 1));
        new_site->animals[new_site->nanimals] = animal;
        new_site->nanimals++;

        pthread_mutex_unlock(&mutex);

        animal->location = new_location;
        printf("Animal moved to location (%d, %d)\n", animal->location.x, animal->location.y);

        /* Additional behavior based on site type*/
        if (new_site->type == WINTERING)
        {
            /*WINTER*/
            double probability = (double)rand() / RAND_MAX;
            if (probability < 0.5)
            {
                animal->status = DEAD;
                printf("Animal died at location (%d, %d)\n", animal->location.x, animal->location.y);
                return NULL;
            }
           
        }
        else if (new_site->type == FEEDING)
        {
            /*FEEDING*/ 
            double probability = (double)rand() / RAND_MAX;
            if (probability >= 0.8)
            {
                /*random location*/   
                int random_x, random_y;
                do {
                    random_x = animal->location.x + rand() % 3 - 1;
                    random_y = animal->location.y + rand() % 3 - 1;
                } while (random_x < 0 || random_x >= grid.xlength || random_y < 0 || random_y >= grid.ylength);
                new_location.x = random_x;
                new_location.y = random_y;

                /*update sites*/ 
                current_side = &grid.sites[animal->location.x][animal->location.y];
                new_site = &grid.sites[new_location.x][new_location.y];

                pthread_mutex_lock(&mutex);

                /*remove animal from current site*/ 
                for (int i = 0; i < current_side->nanimals; i++)
                {
                    if (current_side->animals[i] == animal)
                    {
                        current_side->animals[i] = current_side->animals[current_side->nanimals - 1];
                        current_side->nanimals--;
                        break;
                    }
                }

                /*Add animal to new site*/ 
                new_site->animals = (Animal **)realloc(new_site->animals, sizeof(Animal *) * (new_site->nanimals + 1));
                new_site->animals[new_site->nanimals] = animal;
                new_site->nanimals++;

                pthread_mutex_unlock(&mutex);

                animal->location = new_location;
                printf("Animal moved to location (%d, %d)\n", animal->location.x, animal->location.y);
            }
        }
         else if(new_site->type==NESTING)
            {
            /*NESTING*/
            
                /*random location*/  
                int random_x, random_y;
                do {
                    random_x = animal->location.x + rand() % 3 - 1;
                    random_y = animal->location.y + rand() % 3 - 1;
                } while (random_x < 0 || random_x >= grid.xlength || random_y < 0 || random_y >= grid.ylength);
                new_location.x = random_x;
                new_location.y = random_y;

                /*Update site*/ 
                current_side = &grid.sites[animal->location.x][animal->location.y];
                new_site = &grid.sites[new_location.x][new_location.y];

                pthread_mutex_lock(&mutex);

                /*remove animal from current site*/ 
                for (int i = 0; i < current_side->nanimals; i++)
                {
                    if (current_side->animals[i] == animal)
                    {
                        current_side->animals[i] = current_side->animals[current_side->nanimals - 1];
                        current_side->nanimals--;
                        break;
                    }
                }

                /*Add animal to new site*/ 
                new_site->animals = (Animal **)realloc(new_site->animals, sizeof(Animal *) * (new_site->nanimals + 1));
                new_site->animals[new_site->nanimals] = animal;
                new_site->nanimals++;

                pthread_mutex_unlock(&mutex);

                animal->location = new_location;
                printf("Animal moved to location (%d, %d)\n", animal->location.x, animal->location.y);
            }
    }
}

void *simulatehunter(void *args)
{
    Hunter *hunter = (Hunter *)args;

    while (1)
    {
        int sleepTime = rand() % 3 + 1;
        sleep(sleepTime);

        int x = rand() % 3 - 1;
        int y = rand() % 3 - 1;

        Location new_location = {hunter->location.x + x, hunter->location.y + y};

        /*control for is hunter in grid*/ 
        if (new_location.x < 0 || new_location.x >= grid.xlength || new_location.y < 0 || new_location.y >= grid.ylength)
            continue;

        Site *current_side = &grid.sites[hunter->location.x][hunter->location.y];
        Site *new_site = &grid.sites[new_location.x][new_location.y];

        pthread_mutex_lock(&mutex);

        /*Remove hunter from current site*/ 
        for (int i = 0; i < current_side->nhunters; i++)
        {
            if (current_side->hunters[i] == hunter)
            {
                current_side->hunters[i] = current_side->hunters[current_side->nhunters - 1];
                current_side->nhunters--;
                break;
            }
        }

        /*Add hunter to new site*/ 
        new_site->hunters = (Hunter **)realloc(new_site->hunters, sizeof(Hunter *) * (new_site->nhunters + 1));
        new_site->hunters[new_site->nhunters] = hunter;
        new_site->nhunters++;

        /*Kill animals in the new site and add kills to hunter's points*/ 
        int number_kill = 0;
        for (int i = 0; i < new_site->nanimals; i++)
        {
            Animal *animal = new_site->animals[i];
            animal->status = DEAD;
            number_kill++;
        }
        hunter->points += number_kill;

        /*remove killed animals*/ 
        free(new_site->animals);
        new_site->animals = NULL;
        new_site->nanimals = 0;

        pthread_mutex_unlock(&mutex);

        hunter->location = new_location;
        printf("Hunter moved to location (%d, %d). Killed %d animals. Total points: %d\n",
               hunter->location.x, hunter->location.y, number_kill, hunter->points);
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    int xlength=5;
    int ylength =5;
    

    initgrid(xlength, ylength);

    for (int i = 0; i < 3; i++)
    {
        Animal *animal = (Animal *)malloc(sizeof(Animal));
        animal->status = ALIVE;
        animal->type = (AnimalType)i;
        animal->location.x = rand() % xlength;
        animal->location.y = rand() % ylength;

        grid.sites[animal->location.x][animal->location.y].animals =
            (Animal **)realloc(grid.sites[animal->location.x][animal->location.y].animals,
                               sizeof(Animal *) * (grid.sites[animal->location.x][animal->location.y].nanimals + 1));
        grid.sites[animal->location.x][animal->location.y].animals[grid.sites[animal->location.x][animal->location.y].nanimals] = animal;
        grid.sites[animal->location.x][animal->location.y].nanimals++;

        pthread_create(&animal_threads[i], NULL, simulateanimal, (void *)animal);
    }

    int num_hunters= atoi(argv[1]);
    printf("nhunters=%d",num_hunters);
    

    hunter_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_hunters);

    for (int i = 0; i < num_hunters; i++)
    {
        Hunter *hunter = (Hunter *)malloc(sizeof(Hunter));
        hunter->points = 0;
        hunter->location.x = rand() % xlength;
        hunter->location.y = rand() % ylength;

        grid.sites[hunter->location.x][hunter->location.y].hunters =
            (Hunter **)realloc(grid.sites[hunter->location.x][hunter->location.y].hunters,
                               sizeof(Hunter *) * (grid.sites[hunter->location.x][hunter->location.y].nhunters + 1));
        grid.sites[hunter->location.x][hunter->location.y].hunters[grid.sites[hunter->location.x][hunter->location.y].nhunters] = hunter;
        grid.sites[hunter->location.x][hunter->location.y].nhunters++;

        pthread_create(&hunter_threads[i], NULL, simulatehunter, (void *)hunter);
    }

    printgrid();

    /*Wait for animal threads to finish*/ 
    for (int i = 0; i < 3; i++)
    {
        pthread_join(animal_threads[i], NULL);
    }

    /*Cancel hunter threads*/ 
    for (int i = 0; i < num_hunters; i++)
    {
        pthread_cancel(hunter_threads[i]);
    }

    /*Wait for hunter threads to finish*/ 
    for (int i = 0; i < num_hunters; i++)
    {
        pthread_join(hunter_threads[i], NULL);
    }

    /*clear*/ 
    for (int i = 0; i < grid.xlength; i++)
    {
        for (int j = 0; j < grid.ylength; j++)
        {
            free(grid.sites[i][j].animals);
            free(grid.sites[i][j].hunters);
        }
        free(grid.sites[i]);
    }
    free(grid.sites);
    free(hunter_threads);

    return 0;
}
