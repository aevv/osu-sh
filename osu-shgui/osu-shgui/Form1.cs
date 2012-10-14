using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;
using System.Globalization;
using System.Diagnostics;

namespace osu_shgui
{
    public partial class Form1 : Form
    {
        private string dllName = "";
        private bool nomod, halftime, doubletime, injected = false;
        private double speed = 1;
        IFormatProvider f = new CultureInfo("en-US");
        Process osu = null;
        Injector i = new Injector();
        DLLInformation hook = null;
        [DllImport("psapi.dll")]
        static extern uint GetModuleFileNameEx(IntPtr hProcess, IntPtr hModule, [Out] StringBuilder lpBaseName, [In] [MarshalAs(UnmanagedType.U4)] int nSize);

        public Form1()
        {
            InitializeComponent();
            FileStream fs;
            if (!File.Exists("settings.ini"))
                fs = File.Create("settings.ini");
            else
                fs = new FileStream("settings.ini", FileMode.Open);

            using (StreamReader sr = new StreamReader(fs))
            {
                string line = "";
                int countlol = 0;
                dllName = sr.ReadLine();
                bool got = false;
                while ((line = sr.ReadLine()) != null)
                {
                    bool result = false;
                    double result2 = 0;
                    if (double.TryParse(line, NumberStyles.AllowDecimalPoint, f, out result2))
                    {
                        if (!got)
                        {
                            got = !got;
                            speed = result2;
                        }
                    }
                    else if (bool.TryParse(line, out result))
                    {
                        if (countlol++ == 0)
                        {
                            doubletime = result;
                        }
                        else
                        {
                            halftime = result;
                        }
                    }
                }
                sr.Close();
            }
            if (!halftime && !doubletime)
            {
                nomod = true;
            }
            textBox1.Text = "" + speed;
            radioButton1.Checked = nomod;
            radioButton2.Checked = doubletime;
            radioButton3.Checked = halftime;
            Process[] procs = Process.GetProcesses();
            foreach (Process p in procs)
            {
                if (p.ProcessName == "osu!")
                {
                    osu = p;
                }
            }
            if (osu == null)
            {
                MessageBox.Show("osu not found, closing");
                Environment.Exit(0);
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (!injected)
            {
                if (speed > 1.9)
                {
                    MessageBox.Show("speed must be below 1.9");
                    speed = 1.9;
                }
                if (speed < 0.1)
                {
                    MessageBox.Show("speed must be above 0.1");
                    speed = 0.1;
                }
                write();
                int code = 8;
                try
                {
                    hook = i.inject(osu.Id, dllName);
                    code = hook.ErrorCode;
                }
                catch
                {
                    MessageBox.Show(getError(code));
                }
                if (code != 0)
                {
                    MessageBox.Show(getError(code));
                }
                else
                {
                    injected = true;
                }
            }
            else
            {
                button2_Click(null, null);
                button1_Click(null, null);
            }
        }
        private string getError(int code)
        {
            string res = "injection failed, error code: " + code + "\nError is: ";
            switch (code)
            {
                case 1:
                    res += "cannot open process id (make sure osu!.exe is running)";
                    break;
                case 2:
                    res += "cannot allocate memory in osu!.exes process id (make sure osu!.exe is running)";
                    break;
                case 3:
                    res += "can't write to memory in osu!.exe";
                    break;
                case 4:
                    res += "can't create a thread in osu!.exe";
                    break;
                case 5:
                    res += "cannot get thread exit code";
                    break;
                case 6:
                    res += "load library returned null, probably missing hook dll";
                    break;
                case 7:
                    res += "load library returned null, probably missing hook dll";
                    break;
                case 8:
                    res += "couldn't find injector.dll";
                    break;
            }
            if (code > 0 && code < 9)
                return res;
            return "injection failed, unknown error";
        }
        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            write();
        }
        private void write()
        {
            string dir = Directory.GetCurrentDirectory();
            dir += "\\hook.dll";
            using (StreamWriter sw = new StreamWriter("settings.ini", false))
            {
                sw.WriteLine(dir);
                string s = Convert.ToString(speed, f);
                sw.WriteLine(s);
                sw.WriteLine(doubletime);
                sw.WriteLine(halftime);
                sw.Close();
            }
            string s2 = osu.MainModule.FileName;
            File.Copy("settings.ini", s2.Substring(0, s2.Length - 9) + "\\settings.cfg", true);
        }
        private void radioButton1_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton1.Checked)
            {
                nomod = true;
                halftime = false;
                doubletime = false;
            }
        }

        private void radioButton2_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton2.Checked)
            {
                doubletime = true;
                halftime = false;
                nomod = false;
            }
        }

        private void radioButton3_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton3.Checked)
            {
                halftime = true;
                doubletime = false;
                nomod = false;
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new About().ShowDialog();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Environment.Exit(0);
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            double res = 0;
            if (double.TryParse(textBox1.Text, NumberStyles.AllowDecimalPoint, f, out res))
            {
                speed = res;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (injected)
            {
                i.unject(hook);
                injected = false;
            }
        }
    }
}
