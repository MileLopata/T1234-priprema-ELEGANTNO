/**
 * Napisati C++ program koji cita podatke o uniformnim distribucijama iz ulazne datoteke i potom za
 * svaku distribuciju generise po 10 brojeva, racuna njihov prosek, i nalazi najmanji i najveci broj. 
 * 
 * U ulaznoj datoteci "distribucije.txt" se u svakom redu nalaze informacije o donjoj i gornjoj granici
 * intervala u kojem treba generisati brojeve. Informacije o gornjoj i donjoj granici su odvojene dvotackom.
 *  U pitanju su razlomljeni brojevi.
 * 
 * U izlaznoj datoteci "brojevi.csv" treba da se nalaze u jednom redu odvojeni zarezom prvo 10 
 * izgenerisanih brojeva a potom i prosek, najmanji element i najveci element.
 * 
 * Treba napraviti jednu nit koja ce samo citati ulazne podatke, jednu nit koja ce samo pisati gotove
 * brojeve u datoteku i 6 niti radnika koji ce na osnovu podataka iz ulazne datoteke generisati sve
 * neophodno za ispis u izlaznu datoteku.
*/
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>
#include <random>
using namespace std;
using namespace chrono;

#define BROJ_GENERISANIH_BROJEVA 10
#define BROJ_RADNIKA 6
/** Klasa koja modeluje "postansko sanduce" izmedju citaca i radnika.
*/
template<typename T>
class UlazniPodaci {
private:
    mutex m;
    condition_variable spremni;
    queue<T> kolekcija;
    bool kraj;
public:
    UlazniPodaci(): kraj(false) {}  // na pocetku nije kraj i nema radnika
    void dodaj(T par_granica) {
        unique_lock<mutex> lock(m);
        kolekcija.push(par_granica);
    }
    bool preuzmi(T &par_granica) {
        unique_lock<mutex> lock(m);
        while(kolekcija.empty() && !kraj){
            spremni.wait(lock);
        }
        if(kolekcija.empty() && kraj){
            return false;
        }
        par_granica = kolekcija.front();
        kolekcija.pop();
        return true;
    }
    void objaviKraj(){
        unique_lock<mutex> lock(m);
        kraj = true;
        spremni.notify_all();
    }
};
/** Klasa koja modeluje "postansko sanduce" izmedju radnika i pisaca.
*/
template<typename T>
class IzlazniPodaci {
private:
    mutex m;
    bool kraj;
    condition_variable spremni;
    queue<T> kolekcija;
    int br_stvaralaca_podataka;
public:
    IzlazniPodaci(): kraj(false), br_stvaralaca_podataka(0) {}  // na pocetku nije kraj i nema radnika
    void dodaj(T brojevi) {
        unique_lock<mutex> lock(m);
        kolekcija.push(brojevi);
    }
    bool preuzmi(T &brojevi) {
        unique_lock<mutex> lock(m);
        while(kolekcija.empty() && !kraj){
            spremni.wait(lock);
        }
        if(kolekcija.empty() && kraj){
            return false;
        }
        brojevi = kolekcija.front();
        kolekcija.pop();
        return true;
    }
    void prijavaStvaraocaPodataka(){
        unique_lock<mutex> lock(m);
        br_stvaralaca_podataka++;
    }
    void odjavaStvaraocaPodataka(){
        unique_lock<mutex> lock(m);
        br_stvaralaca_podataka--;
        if(br_stvaralaca_podataka == 0){
            kraj = true;
            spremni.notify_one();
        }
    }
};
/**
 * Logika radnika - niti koje vrse transformaciju ulaznih podataka u izlazne podatke spremne za ispis.
*/
void radnik(UlazniPodaci<pair<double, double>> &ulaz, IzlazniPodaci<vector<double>> &izlaz) {
    izlaz.prijavaStvaraocaPodataka();
    pair<double, double> par_granica;
    vector<double> brojevi(13);
    default_random_engine gen;
    gen.seed(system_clock::now().time_since_epoch().count());
    while(ulaz.preuzmi(par_granica)){
        uniform_real_distribution<double> dist(par_granica.first, par_granica.second);
        for(int i = 0; i < 10; i++){
            brojevi[i] = dist(gen);
        }
        double min, max, sum;
        min = brojevi[0];
        for(int i = 1; i < 10; i++){
            if(brojevi[i] < min){
                min = brojevi[i];
            }
        }
        max = brojevi[0];
        for(int i = 1; i < 10; i++){
            if(brojevi[i] > max){
                max = brojevi[i];
            }
        }
        sum = 0;
        for(int i = 0; i < 10; i++){
            sum += brojevi[i];
        }
        double avg = sum / 10;
        brojevi[10] = min;
        brojevi[11] = max;
        brojevi[12] = avg;
        izlaz.dodaj(brojevi);
    }
    izlaz.odjavaStvaraocaPodataka();
}

/**
 * Logika citaca_iz_datoteke - nit koja radi citanje iz ulazne datoteke i salje u ulaznu kolekciju za radnike
*/
void citacf(string ime_ulazne_dat, UlazniPodaci<pair<double, double>> &ulaz) {
    ifstream ulazna_dat(ime_ulazne_dat);
    string line;
    if(ulazna_dat.is_open()){
        while(getline(ulazna_dat, line)){
            istringstream str(line);
            double donjaGranica, gornjaGranica;
            string donjaG, gornjaG;
            getline(str, donjaG, ':');
            getline(str, gornjaG, ':');
            donjaGranica = stod(donjaG);
            gornjaGranica = stod(gornjaG);
            pair<double, double> par(donjaGranica, gornjaGranica);
            ulaz.dodaj(par);
            //cout<<par.first<<" "<<par.second<<endl;
        }
    }
    ulaz.objaviKraj();
    ulazna_dat.close();
}
/**
 * Logika pisaca_u_datoteku - nit koja radi pisanje u izlaznu datoteku podataka dobijenih od radnika
*/
void pisacf(IzlazniPodaci<vector<double>> &izlaz, string ime_izlazne_dat) {
    ofstream izlazna_dat(ime_izlazne_dat);
    vector<double> brojevi;
    if(izlazna_dat.is_open()){
        for(int i = 0; i < 10; i++){
            izlazna_dat<<i+1<<".,";
            cout<<i+1<<".,";
        }
        izlazna_dat<<"min, max, avg"<<endl;;
        cout<<"min, max, avg"<<endl;
        while(izlaz.preuzmi(brojevi)){
            for(int i = 0; i < 13; i++){
                izlazna_dat<<brojevi[i]<<",";
                cout<<brojevi[i]<<",";
            }
            izlazna_dat<<endl;
            cout<<endl;
        }
    }
    izlazna_dat.close();
}

int main() {
    UlazniPodaci<pair<double, double>> ulazni_podaci;  // bafer podataka koje salje citac_iz_datoteke a obradjuju radnici
    IzlazniPodaci<vector<double>> izlazni_podaci;  // bafer podataka koje pripremaju radnici a ispisuju se u datoteku u pisacu_u_datoteku
    thread citac{citacf, "distribucije.txt", ref(ulazni_podaci)},  // stvaranje niti citaca_iz_datoteke
           pisac{pisacf, ref(izlazni_podaci), "brojevi.csv"},      // stvaranje niti pisaca_u_datoteku
           radnici[6];

    for (auto &nit: radnici)
        nit = thread(radnik, ref(ulazni_podaci), ref(izlazni_podaci));  // stvaranje niti radnika

    for (auto &nit: radnici)
        nit.join();
    citac.join();
    pisac.join();

    return 0;
}
