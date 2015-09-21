using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Ini;
using System.Diagnostics;

namespace Orco_Configuration
{
    public partial class Form1 : Form
    {
        /// Imagen de la gondola desde arriba
        Bitmap image;
        String image_path;
        /// ROI de Gondola
        int xMinROI, yMinROI, xMaxROI, yMaxROI;
        // evento para manejar los cambios de valores de la gondola
        System.EventHandler ROI_valueChange;
        System.EventHandler PASILLO_valueChange;
        /// Límites del pasillo
        int xMinLineLeft, yMinLineLeft, xMaxLineLeft, yMaxLineLeft, xMinLineRight, yMinLineRight, xMaxLineRight, yMaxLineRight;
        /// CSV file
        String csv_file_path;
        bool save_csv_file;
        /// Video directory
        bool save_video;
        String video_directory_path;
        /// Kinect
        bool readFromKinect;

        /// Property File
        IniFile propertyFile;
        IniFile propertyROIFile;
        String ROI_FILE_PATH = "ROI_FILE_PATH";
        /// Secciones
        String SETTING_SECTION = "Setting";
        String VIDEO_SECTION = "VIDEO";
        String CSV_SECTION = "CSV";
        String INFO_SECTION = "info";
        String ROI_SECTION = "ROI";
        String KINECT_SECTION = "KINECT";
        /// Nombre de las propiedades
        String IMAGE_PATH = "GONDOLA_IMAGE_FILE_PATH";
        String CSV_FILE_PATH = "CSV_FILE_PATH";
        String SAVE_CSV_FILE = "SAVE_CSV_FILE";
        String SAVE_VIDEO = "SAVE_VIDEO";
        String VIDEO_DIRECTORY_PATH = "VIDEO_DIRECTORY_PATH";
        String READ_FROM_KINECT = "READ_FROM_KINECT";

        public Form1()
        {
            InitializeComponent();
            propertyFile = new IniFile(".\\Configuration.ini");
            String filePath = propertyFile.IniReadValue(INFO_SECTION, ROI_FILE_PATH);
            if (filePath == null || filePath.Length == 0)
            {
                toolStripStatusLabel1.Text = ".\\Settings.ini";
                propertyFile.IniWriteValue(INFO_SECTION, ROI_FILE_PATH, toolStripStatusLabel1.Text);
            }
            else
                toolStripStatusLabel1.Text = filePath;
            updatePropertiesFromFile();
            
        }

        /// <summary>
        /// Actualiza todas las propiedades leyendolas desde el archivo de configuración
        /// </summary>
        public void updatePropertiesFromFile()
        {
            // Cargo archivo de propiedades
            propertyROIFile = new IniFile(toolStripStatusLabel1.Text);
            readROIInformation();
            readCSVInformation();
            readVideoInformation();
            readKinectInformation();
            // Agrego los eventos a los numericupdown para que actualicen el archivo de propiedades cuando haya una modificacion
            ROI_valueChange = new System.EventHandler(this.numericUpDown4_ValueChanged);
            this.numericUpDown_right.ValueChanged += ROI_valueChange;
            this.numericUpDown_left.ValueChanged += ROI_valueChange;
            this.numericUpDown_top.ValueChanged += ROI_valueChange;
            this.numericUpDown_bottom.ValueChanged += ROI_valueChange;
            PASILLO_valueChange += new System.EventHandler(this.numericUpDown_xtl_ValueChanged);
            this.numericUpDown_xtl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ytl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xbl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ybl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xbr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ybr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xtr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ytr.ValueChanged += PASILLO_valueChange;
            // imagen de la gondola desde arriba
            image_path = propertyFile.IniReadValue(INFO_SECTION, IMAGE_PATH);
            if (image_path != null && image_path.Length > 0)
            {
                loadImageFromFile();
            }
        }

        private void readKinectInformation()
        {
            String aux = propertyROIFile.IniReadValue(KINECT_SECTION, READ_FROM_KINECT);
            if (aux.Length > 0)
            {
                readFromKinect = Boolean.Parse(aux);
            }
            else
            {
                readFromKinect = true;
                propertyROIFile.IniWriteValue(KINECT_SECTION, READ_FROM_KINECT, readFromKinect.ToString());
            }
            capturarDesdeKinectToolStripMenuItem.Checked = readFromKinect;
        }


        /// <summary>
        /// Lee los valores de la góndola desde el archivo de propiedades
        /// </summary>
        public void readROIInformation()
        {
            numericUpDown_right.ValueChanged -= ROI_valueChange;
            numericUpDown_left.ValueChanged -= ROI_valueChange;
            numericUpDown_top.ValueChanged -= ROI_valueChange;
            numericUpDown_bottom.ValueChanged -= ROI_valueChange;
            this.numericUpDown_xtl.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_ytl.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_xbl.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_ybl.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_xbr.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_ybr.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_xtr.ValueChanged -= PASILLO_valueChange;
            this.numericUpDown_ytr.ValueChanged -= PASILLO_valueChange;
            try
            {
                numericUpDown_left.Value = Int32.Parse(propertyROIFile.IniReadValue(ROI_SECTION, "xMinROI"));
                numericUpDown_right.Value = Int32.Parse(propertyROIFile.IniReadValue(ROI_SECTION, "xMaxROI"));
                numericUpDown_top.Value = Int32.Parse(propertyROIFile.IniReadValue(ROI_SECTION, "yMinROI"));
                numericUpDown_bottom.Value = Int32.Parse(propertyROIFile.IniReadValue(ROI_SECTION, "yMaxROI"));
                this.numericUpDown_xtl.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "xMinLineLeft"));
                this.numericUpDown_ytl.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "yMinLineLeft"));
                this.numericUpDown_xbl.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "xMaxLineLeft"));
                this.numericUpDown_ybl.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "yMaxLineLeft"));
                this.numericUpDown_xbr.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "xMaxLineRight"));
                this.numericUpDown_ybr.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "yMaxLineRight"));
                this.numericUpDown_xtr.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "xMinLineRight"));
                this.numericUpDown_ytr.Value = Int32.Parse(propertyROIFile.IniReadValue(SETTING_SECTION, "yMinLineRight"));
            }catch(FormatException e){
                numericUpDown_left.Value = 0;
                numericUpDown_right.Value = 640;
                numericUpDown_top.Value = 320;
                numericUpDown_bottom.Value = 480;
                this.numericUpDown_xtl.Value = 10;
                this.numericUpDown_ytl.Value = 10;
                this.numericUpDown_xbl.Value = 10;
                this.numericUpDown_ybl.Value = 470;
                this.numericUpDown_xbr.Value = 630;
                this.numericUpDown_ybr.Value = 470;
                this.numericUpDown_xtr.Value = 630;
                this.numericUpDown_ytr.Value = 10;
            }
            xMinROI = Decimal.ToInt32(numericUpDown_left.Value);
            xMaxROI = Decimal.ToInt32(numericUpDown_right.Value);
            yMinROI = Decimal.ToInt32(numericUpDown_top.Value);
            yMaxROI = Decimal.ToInt32(numericUpDown_bottom.Value);
            xMinLineLeft = Decimal.ToInt32(numericUpDown_xtl.Value);
            yMinLineLeft = Decimal.ToInt32(numericUpDown_ytl.Value);
            xMaxLineLeft = Decimal.ToInt32(numericUpDown_xbl.Value);
            yMaxLineLeft = Decimal.ToInt32(numericUpDown_ybl.Value);
            xMinLineRight = Decimal.ToInt32(numericUpDown_xtr.Value);
            yMinLineRight = Decimal.ToInt32(numericUpDown_ytr.Value);
            xMaxLineRight = Decimal.ToInt32(numericUpDown_xbr.Value);
            yMaxLineRight = Decimal.ToInt32(numericUpDown_ybr.Value);
            this.numericUpDown_right.ValueChanged += ROI_valueChange;
            this.numericUpDown_left.ValueChanged += ROI_valueChange;
            this.numericUpDown_top.ValueChanged += ROI_valueChange;
            this.numericUpDown_bottom.ValueChanged += ROI_valueChange;
            this.numericUpDown_xtl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ytl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xbl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ybl.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xbr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ybr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_xtr.ValueChanged += PASILLO_valueChange;
            this.numericUpDown_ytr.ValueChanged += PASILLO_valueChange;
        }

        public void readVideoInformation()
        {
            String aux = propertyROIFile.IniReadValue(VIDEO_SECTION, SAVE_VIDEO);
            if (aux.Length > 0)
                save_video = Boolean.Parse(aux);
            else
            {
                save_video = true;
                propertyROIFile.IniWriteValue(VIDEO_SECTION, SAVE_VIDEO, save_video.ToString());
            }
            guardarVideosToolStripMenuItem.Checked = save_video;
            video_directory_path = propertyROIFile.IniReadValue(VIDEO_SECTION, VIDEO_DIRECTORY_PATH);
            if (video_directory_path.Length == 0)
            {
                video_directory_path = "videos";
                propertyROIFile.IniWriteValue(VIDEO_SECTION, VIDEO_DIRECTORY_PATH, video_directory_path);
            }
        }

        public void readCSVInformation()
        {
            String aux = propertyROIFile.IniReadValue(CSV_SECTION, SAVE_CSV_FILE);
            if (aux.Length > 0)
                save_csv_file = Boolean.Parse(aux);
            else
            {
                save_csv_file = true;
                propertyROIFile.IniWriteValue(CSV_SECTION, SAVE_CSV_FILE, save_csv_file.ToString());
            }
            generarArchivosCSVToolStripMenuItem.Checked = save_csv_file;
            csv_file_path = propertyROIFile.IniReadValue(CSV_SECTION, CSV_FILE_PATH);
            if (csv_file_path.Length == 0)
            {
                csv_file_path = "orco.csv";
                propertyROIFile.IniWriteValue(CSV_SECTION, CSV_FILE_PATH, csv_file_path);
            }
        }

        public void drawLimites()
        {
            if (image_path != null && image_path.Length > 0)
                image = new Bitmap(image_path);

            drawROI();
            drawPASILLO();

            pictureBox1.Image = image;
            pictureBox1.Refresh();
        }

        /// <summary>
        /// Dibuja sobre la imagen el ROI
        /// </summary>
        public void drawROI()
        {
            if (checkBox_mostrar_Gondola.Checked)
            {
                Pen pen = new Pen(Color.Yellow, 2);
                using (var graphics = Graphics.FromImage(image))
                {
                    graphics.DrawLine(pen, new Point(xMinROI, yMinROI), new Point(xMinROI, yMaxROI));
                    graphics.DrawLine(pen, new Point(xMinROI, yMaxROI), new Point(xMaxROI, yMaxROI));
                    graphics.DrawLine(pen, new Point(xMaxROI, yMaxROI), new Point(xMaxROI, yMinROI));
                    graphics.DrawLine(pen, new Point(xMaxROI, yMinROI), new Point(xMinROI, yMinROI));
                }
            }
        }

        public void drawPASILLO()
        {
            if (checkBox_mostrar_pasillo.Checked)
            {
                Pen pen = new Pen(Color.Red, 2);
                using (var graphics = Graphics.FromImage(image))
                {
                    graphics.DrawLine(pen, new Point(xMinLineLeft, yMinLineLeft), new Point(xMaxLineLeft, yMaxLineLeft));
                    graphics.DrawLine(pen, new Point(xMinLineRight, yMinLineRight), new Point(xMaxLineRight, yMaxLineRight));
                }
            }
        }

        /// <summary>
        /// Método invocado cuando cambia alguno de los valores de los límites de la góndola
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void numericUpDown4_ValueChanged(object sender, EventArgs e)
        {
            xMinROI = Decimal.ToInt32(numericUpDown_left.Value);
            xMaxROI = Decimal.ToInt32(numericUpDown_right.Value);
            yMinROI = Decimal.ToInt32(numericUpDown_top.Value);
            yMaxROI = Decimal.ToInt32(numericUpDown_bottom.Value);

            propertyROIFile.IniWriteValue(ROI_SECTION, "xMinROI", xMinROI.ToString());
            propertyROIFile.IniWriteValue(ROI_SECTION, "xMaxROI", xMaxROI.ToString());
            propertyROIFile.IniWriteValue(ROI_SECTION, "yMinROI", yMinROI.ToString());
            propertyROIFile.IniWriteValue(ROI_SECTION, "yMaxROI", yMaxROI.ToString());

            if (image != null)
                drawLimites();
        }

        /// <summary>
        /// Busca la ubicación de la imagen en disco
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button1_Click(object sender, EventArgs e)
        {
            if (openFileDialog_image.ShowDialog() == DialogResult.OK)
            {
                image_path = openFileDialog_image.FileName;
                loadImageFromFile();
                propertyFile.IniWriteValue(INFO_SECTION, IMAGE_PATH, image_path);
            }
        }


        /// <summary>
        /// Muestra la imagen en pantalla a partir de la ubicación guardada en la variable image_path
        /// </summary>
        public void loadImageFromFile()
        {
            image = new Bitmap(image_path);
            pictureBox1.Image = image;
            pictureBox1.Refresh();
            drawLimites();
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            drawLimites();
        }

        private void abrirToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (openFileDialog_property.ShowDialog() == DialogResult.OK)
            {
                toolStripStatusLabel1.Text = openFileDialog_property.FileName;
                propertyFile.IniWriteValue(INFO_SECTION, ROI_FILE_PATH, toolStripStatusLabel1.Text);
                updatePropertiesFromFile();
            }
        }

        private void guardarToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                toolStripStatusLabel1.Text = saveFileDialog1.FileName;
                propertyFile.IniWriteValue(INFO_SECTION, ROI_FILE_PATH, toolStripStatusLabel1.Text);
            }

        }

        /// <summary>
        /// Método invocado cuando uno de los parámetros de los límites del pasillo es modificado.
        /// Actualiza el archivo de propiedades y vuelve a dibujar las líneas en la imagen.
        /// Si anteriormente se mostro el ROI, entonces no vuelve a cargar la imagen original.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void numericUpDown_xtl_ValueChanged(object sender, EventArgs e)
        {
            xMinLineLeft = Decimal.ToInt32(numericUpDown_xtl.Value);
            yMinLineLeft = Decimal.ToInt32(numericUpDown_ytl.Value);
            xMaxLineLeft = Decimal.ToInt32(numericUpDown_xbl.Value);
            yMaxLineLeft = Decimal.ToInt32(numericUpDown_ybl.Value);
            xMinLineRight = Decimal.ToInt32(numericUpDown_xtr.Value);
            yMinLineRight = Decimal.ToInt32(numericUpDown_ytr.Value);
            xMaxLineRight = Decimal.ToInt32(numericUpDown_xbr.Value);
            yMaxLineRight = Decimal.ToInt32(numericUpDown_ybr.Value);

            propertyROIFile.IniWriteValue(SETTING_SECTION, "xMinLineLeft", xMinLineLeft.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "yMinLineLeft", yMinLineLeft.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "xMaxLineLeft", xMaxLineLeft.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "yMaxLineLeft", yMaxLineLeft.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "xMinLineRight", xMinLineRight.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "yMinLineRight", yMinLineRight.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "xMaxLineRight", xMaxLineRight.ToString());
            propertyROIFile.IniWriteValue(SETTING_SECTION, "yMaxLineRight", yMaxLineRight.ToString());
            if (image != null)
                drawLimites();
        }

        private void checkBox1_CheckedChanged_1(object sender, EventArgs e)
        {
            drawLimites();
        }

        private void ubicaciónToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveCSVFileDialog2.FileName = csv_file_path;
            if (saveCSVFileDialog2.ShowDialog() == DialogResult.OK)
            {
                csv_file_path = saveCSVFileDialog2.FileName;
                propertyROIFile.IniWriteValue(CSV_SECTION, CSV_FILE_PATH, csv_file_path);
            }
        }

        private void generarArchivosCSVToolStripMenuItem_Click(object sender, EventArgs e)
        {
            save_csv_file = generarArchivosCSVToolStripMenuItem.Checked;
            propertyROIFile.IniWriteValue(CSV_SECTION, SAVE_CSV_FILE, save_csv_file.ToString());
        }

        private void guardarVideosToolStripMenuItem_Click(object sender, EventArgs e)
        {
            save_video = guardarVideosToolStripMenuItem.Checked;
            propertyROIFile.IniWriteValue(VIDEO_SECTION, SAVE_VIDEO, save_video.ToString());
        }

        private void ubicaciónVideosToolStripMenuItem_Click(object sender, EventArgs e)
        {
            videoBrowserDialog.SelectedPath = video_directory_path;
            if (videoBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                video_directory_path = videoBrowserDialog.SelectedPath;
                propertyROIFile.IniWriteValue(VIDEO_SECTION, VIDEO_DIRECTORY_PATH, video_directory_path);
            }
        }

        private void capturarDesdeKinectToolStripMenuItem_Click(object sender, EventArgs e)
        {
            readFromKinect = capturarDesdeKinectToolStripMenuItem.Checked;
            propertyROIFile.IniWriteValue(KINECT_SECTION, READ_FROM_KINECT, readFromKinect.ToString());
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (saveImageFromKinectDialog.ShowDialog() == DialogResult.OK)
            {
                Process p = new Process();
                // Redirect the output stream of the child process.
                p.StartInfo.UseShellExecute = true;
                p.StartInfo.RedirectStandardOutput = false;

                p.StartInfo.FileName = Environment.CurrentDirectory + "\\BlobsDetection.exe";
                p.StartInfo.Arguments = "-S\"" + saveImageFromKinectDialog.FileName + "\" -C\"C:\\Proyecto ORCO\\app\\Settings.ini\" -D\"C:\\Proyecto ORCO\\app\\DB.ini\"";
                MessageBox.Show(p.StartInfo.FileName+ " "+p.StartInfo.Arguments);
                p.Start();
                p.WaitForExit();
            }
        }

    }
}
