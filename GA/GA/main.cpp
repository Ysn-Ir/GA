#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <SFML/Graphics.hpp>
sf::View v(sf::Vector2f(0, 0), sf::Vector2f(1500, 1500));
//window size
const int w = 800, l = 600;
//rocket size
sf::Vector2f rocketSize(30, 5);
//basically the range of the rand function and how fast is the rockets move
const float speed = 0.01;
//how many frames a generation has need to figure out how to optimize the model
const int frames = 5000;
//size of the generation
const int size = 100;
//Target position
const sf::Vector2f TargetPos(w/2 , 100);
//this function generates a random float between -speed ,+speed
float getRandomFloat();
//this function generates a random int between min ,max
int getRandomInt(int min, int max);

class Dna {
public:
    //this is the vector that holdes all the mouvemnt of a rocket in which we called it a gene , and it size is equal to frames , 
    // due to the life span of each rocket is represented by the frame in rocket class 
    std::vector<sf::Vector2f> gene;
    // mutation rate is the probabilite that a gene might get a random change after the crossover
    float mutationRate = 1;//percentage <0,1>
    Dna() {
        for (int i = 0; i < frames; i++)
            gene.push_back(sf::Vector2f(getRandomFloat(), getRandomFloat()));
    }
    //crossover ,there is planty of methods bbut i prefer keeping it a random position in the gene vector , after that exchange the genetical information between (this gene) 
    // and the (B gene) 
    Dna crossover(const Dna& B) {
        Dna newdna;
        int midpoint = getRandomInt(0, frames);
        for (int i = 0; i < frames; i++) {
            if (i < midpoint)
                newdna.gene[i] = this->gene[i];
            else
                newdna.gene[i] = B.gene[i];
        }
        return newdna;
    }
    //mutation :: pretty clear i guess
    void mutate() {
        for (int i = 0; i < gene.size(); i++) {
            float x = getRandomFloat();//        <-0.01,0.01>
            if ( x< mutationRate && -mutationRate>x) {
                this->gene[i] = sf::Vector2f(getRandomFloat(), getRandomFloat());
            }
        }
    }
};

class Rocket {
private:
    // we first create the positionning of the rocket , it clear
    sf::Vector2f pos, vel, acc;
    //this is the frame which represent the life span of each rocket
    int frame;
    //just to draw an instance , keep in it in the stack is faster that creating it and deleting it as a dynamic memory each frame
    sf::RectangleShape shape;
public:
    //our gene vector
    Dna dna;
    //this is the fitness which represent how good is the rocket (in here its how close to the target in the last frame)
    float fitness;
    //all about movement
    bool fixed=0;
    //constructors
    Rocket() : shape(rocketSize), pos(w / 2, l - 25), frame(0), fitness(0) {
        shape.setOrigin(shape.getSize().x / 2, shape.getSize().y / 2);
    }
    Rocket(Dna dna) : shape(rocketSize), pos(w / 2, l - 25), frame(0), fitness(0), dna(dna) {
        shape.setOrigin(shape.getSize().x / 2, shape.getSize().y / 2);
    }
    //updating the position of the rocket
    void update() {
        this->acc += dna.gene.at(frame);
        if (!fixed) {
            this->vel += acc;
            this->pos += vel;
            this->acc = sf::Vector2f(0, 0);
        }
        checkcollision();
        frame++;
    }
    void checkcollision() {
        float distance = sqrtf(pow(this->pos.x - TargetPos.x, 2) + pow(this->pos.y - TargetPos.y, 2));
        if (pos.x > w || pos.x<0 || pos.y>l || pos.y < 0) {
            fixed = 1;
            fitness /= 5;
        }
        else if (distance <= 30) {
            fixed = 1;
            this->fitness *= 3;
        }
  
    }
    int getFrames() {
        return this->frame;
    }
    //just draw the rocket
    void render(sf::RenderWindow* window) {
        shape.setRotation(float(atan2(vel.y, vel.x) * 180 / 3.14159));
        shape.setPosition(this->pos);
        window->draw(shape);
    }
    //reset the rocket instances after it dies
    void reset() {
        this->pos = sf::Vector2f(w / 2, l - 25);
        frame = 0;
        this->vel = sf::Vector2f(0, 0);
    }
    sf::Vector2f getPos() {
        return pos;
    }
};
//Population is represented as the solution set , in this case just a vector of rockets
class Population {
    std::vector<Rocket> rockets;
public:
    //constructor 
    Population() {
        for (int i = 0; i < size; i++) {
            rockets.push_back(Rocket());
        }
    }
    //drawing everything and updating every rocket 
    void RenderAll(sf::RenderWindow* window) {
        for (int i = 0; i < size; i++) {
            rockets[i].update();
            rockets[i].render(window);
        }
    }
    int getFrames() {
        return rockets[0].getFrames();
    }
    void reset() {
        for (int i = 0; i < size; i++) {
            rockets[i].reset();
        }
    }
    ///////////////////////////////////////////////////////genetic algorithms start here//////////////////////////////////////////////////////////////////////////////////
   //first we calculate the fitness of each rocket 
    void calculateFitness() {
        float sumfit = 0;
        for (Rocket& i : rockets) {
            i.fitness = 1 / ( 0.01+sqrtf(pow(i.getPos().x - TargetPos.x, 2) + pow(i.getPos().y - TargetPos.y, 2)));
            sumfit += i.fitness;
        }
        std::cout << "average fitness = " << sumfit/size << std::endl;
        for (Rocket& i : rockets) {
            i.fitness /= sumfit;
        }
    }
    //selection
    // here we select the best rockets that we will choose for the next generation
    void selection() {
        //here we will put all the rocket but now , we will add n of each rocket , here n represented by the fitness of each one , so if the fitness of the ith rocket is 0
        // we will add it only once , and if its fitness is 1 we will add it 100th times
        std::vector<Rocket> fitRockets;
        for (int i = 0; i < size; i++) {
            int n = rockets[i].fitness * 100;
            for (int j = 0; j < n; j++) {
                fitRockets.push_back(rockets[i]);
            }
        }
        // here we will choose randomly from the fitrockets vector ,to repopulate the next gen after making crossover and mutation ofc
        for (int i = 0; i < size; i++) {
            Dna parentA = fitRockets[getRandomInt(0, fitRockets.size() - 1)].dna;
            Dna parentB = fitRockets[getRandomInt(0, fitRockets.size() - 1)].dna;
            Dna child = parentA.crossover(parentB);
            child.mutate();
            rockets[i] = Rocket(child);
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(w, l), "Rockets", sf::Style::Default);
    //creating instance of the population
    Population pop;
    //the target
    sf::CircleShape cir(30.f);
    cir.setPosition(sf::Vector2f(TargetPos));
    cir.setOrigin(  sf::Vector2f(30, 30));
    cir.setFillColor(sf::Color::Red);
    while (window.isOpen()) {
        //poll event
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                break;
            }
        }

        // Update the population
        if (pop.getFrames() >= frames) {
            //population processing
            pop.calculateFitness();
            pop.selection();
            pop.reset();
        }

        // Render
        //window.setView(v);
        window.clear();
        window.draw(cir);
        pop.RenderAll(&window);
        window.display();
    }

    return 0;
}

float getRandomFloat() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-speed, speed);
    return dist(gen);
}

int getRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}
