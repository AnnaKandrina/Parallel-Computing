using System;
using System.Collections.Generic;
using System.Data;
using System.Threading;

public class DiningPhilosophers
{
    private int numPhilosophers;
    private int msDurationOfEating;
    private Mutex[] forks;
    private volatile bool[] isEating;
    private Random random;
    private DataTable table;
    private Dictionary<int, TimeSpan> totalEatingTimes;

    public DiningPhilosophers(int numPhilosophers, int durationOfEating)
    {
        this.numPhilosophers = numPhilosophers;
        this.msDurationOfEating = durationOfEating * 1000;
        forks = new Mutex[numPhilosophers];
        isEating = new bool[numPhilosophers];
        random = new Random();
        table = new DataTable();
        totalEatingTimes = new Dictionary<int, TimeSpan>();

        for (int i = 0; i < numPhilosophers; i++)
        {
            forks[i] = new Mutex();
            isEating[i] = false;
            table.Columns.Add($"Philosopher {i + 1}", typeof(TimeSpan));
            totalEatingTimes[i] = TimeSpan.Zero;
        }
    }

    private void Think(int philosopherId)
    {
        // Философ размышляет
        Console.WriteLine($"Philosopher {philosopherId + 1} is thinking");
        //Thread.Sleep(this.msDurationOfEating / this.numPhilosophers);
    }

    private void Eat(int philosopherId)
    {
        // Философ ест
        Console.WriteLine($"Philosopher {philosopherId + 1} is eating");
   //     Thread.Sleep(this.msDurationOfEating / this.numPhilosophers); // Случайная задержка приема пищи
    }

    public void StartDining()
    {
        Timer timer = new Timer(_ =>
        {
            StopDining();
        }, null, this.msDurationOfEating, Timeout.Infinite);

        Thread[] philosophers = new Thread[numPhilosophers];
        DateTime[] startTimes = new DateTime[numPhilosophers];

        for (int i = 0; i < numPhilosophers; i++)
        {
            int philosopherId = i;
            philosophers[i] = new Thread(() =>
            {
                while (!timerDisposed && !timerExpired)
                {
                    Think(philosopherId);

                    if (philosopherId == 0)
                    {
                        forks[(philosopherId + 1) % numPhilosophers].WaitOne();
                        forks[philosopherId].WaitOne();
                    }
                    else
                    {
                        forks[philosopherId].WaitOne();
                        forks[(philosopherId + 1) % numPhilosophers].WaitOne();
                    }

                    if (!isEating[philosopherId] && !isEating[(philosopherId + 1) % numPhilosophers])
                    {
                        isEating[philosopherId] = true;
                        isEating[(philosopherId + 1) % numPhilosophers] = true;

                        forks[philosopherId].ReleaseMutex();
                        forks[(philosopherId + 1) % numPhilosophers].ReleaseMutex();

                        startTimes[philosopherId] = DateTime.Now;

                        Eat(philosopherId);

                        TimeSpan elapsedTime = DateTime.Now - startTimes[philosopherId];
                        totalEatingTimes[philosopherId] += elapsedTime;

                        forks[philosopherId].WaitOne();
                        forks[(philosopherId + 1) % numPhilosophers].WaitOne();

                        isEating[philosopherId] = false;
                        isEating[(philosopherId + 1) % numPhilosophers] = false;

                        forks[philosopherId].ReleaseMutex();
                        forks[(philosopherId + 1) % numPhilosophers].ReleaseMutex();
                    }
                    else
                    {
                        forks[philosopherId].ReleaseMutex();
                        forks[(philosopherId + 1) % numPhilosophers].ReleaseMutex();
                    }
                }
            });

            philosophers[i].Start();
        }

        for (int i = 0; i < numPhilosophers; i++)
        {
            philosophers[i].Join();
        }

        Console.WriteLine("\nTime Statistics: ");
        Console.WriteLine($"Number        Time             Percentage");
        
        foreach (var kvp in totalEatingTimes)
        {
            long hours2ms = kvp.Value.Hours * 60 * 60 * 1000;
            long minutes2ms = kvp.Value.Minutes * 60 * 1000 ;
            long seconds2ms = kvp.Value.Seconds * 1000;
            long totalTime = kvp.Value.Milliseconds + seconds2ms + minutes2ms + hours2ms;
            Console.WriteLine($"Philosopher {kvp.Key + 1} {kvp.Value} {totalTime * 100 / this.msDurationOfEating}%");
        }
    }

    private bool timerDisposed = false;
    private bool timerExpired = false;

    private void StopDining()
    {
        timerExpired = true;

        // Дополнительные действия при завершении программы

        // Освобождение ресурсов и остановка потоков

        timerDisposed = true;
    }
}

public class Program
{
    public static void Main(string[] args)
    {
        Console.Write("Enter the number of philosophers: ");
        int numPhilosophers = int.Parse(Console.ReadLine());
        Console.Write("Enter the duration of eating [seconds]: ");
        int durationOfEating = int.Parse(Console.ReadLine());

        DiningPhilosophers dining = new DiningPhilosophers(numPhilosophers, durationOfEating);
        dining.StartDining();
    }
}
